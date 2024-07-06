#include <asl/FtpClient.h>
#include <asl/String.h>
#include <asl/File.h>
#include <asl/Map.h>
#include <asl/Path.h>
#ifdef ASL_TLS
#include <asl/TlsSocket.h>
#endif

namespace asl
{
inline bool ansOK(int n)
{
	return n >= 200 && n < 300;
}

static DirEntry parseDirEntry(const String& line)
{
	DirEntry item;
	item.size = 0;
	item.isdir = false;
	auto parts = line.toLowerCase().split(';', '=');
	if (!line || !parts)
		return item;
	item.name = line.substring(line.lastIndexOf("; ") + 2);
	item.isdir = parts["type"] == "dir";
	item.size = parts["size"];
	String date = parts["modify"];
	item.date = date.length() >= 14 ? Date(Date::UTC, date.substr(0, 4), date.substr(4, 2), date.substr(6, 2),
	                                       date.substr(8, 2), date.substr(10, 2), date.substr(12, 2))
	                                : Date();
	return item;
}

FtpClient::FtpClient()
{
	_hasMlsd = false;
}

bool FtpClient::connect(const String& host, int port)
{
	_host = host;
	_socket = Socket();

	int sep = host.indexOf("://");
	if (sep > 0)
	{
		String proto = host.substring(0, sep);
		_host = host.substr(sep + 3);
		if (proto == "ftps")
		{
#ifdef ASL_TLS
			if (port < 0)
				port = 990;
			_socket = TlsSocket();
#else
			return false;
#endif
		}
	}

	if (port < 0)
		port = 21;

	bool ok = _socket.connect(_host, port);
	if (!ok)
		return false;

	auto ans = getReply();

	return true;
}

bool FtpClient::login(const String& user, const String& pass)
{
	if (!_socket)
		return false;
	
	auto ans = sendCommand("USER " + (user | "anonymous"));
	
	if (ans.first != 331)
		return false;
	
	ans = sendCommand("PASS " + (pass | "anonymous@here.net"));
	
	if (!ansOK(ans.first))
		return false;

	ans = sendCommand("FEAT");
	if (ansOK(ans.first))
	{
		auto feats = ans.second.split();
		_hasMlsd = feats.contains("MLST") || feats.contains("MLSD");
	}

	return true;
}

void FtpClient::disconnect()
{
	_socket = Socket();
	_socketData = Socket();
	_host = "";
	_hasMlsd = false;
}

bool FtpClient::cd(const String& dir)
{
	auto ans = sendCommand("CWD " + dir);
	return ansOK(ans.first);
}

Array<DirEntry> FtpClient::list(const String& dir)
{
	Array<DirEntry> items;

	if (!_socket || !startPassive())
		return items;

	auto ans = sendCommand((_hasMlsd ? "MLSD " : "LIST ") + dir);

	if (ans.first > 299)
		return items;

	String str;

	while (_socketData.connected())
		str << String(_socketData.read(2000));

	auto lines = str.split("\r\n");

	if (_hasMlsd)
	{
		for (auto& line : lines)
		{
			auto item = parseDirEntry(line);
			if (item.name && item.name != "." && item.name != "..")
				items << item;
		}
	}
	else
	{
		Dic<int> months = { { "Jan", 1 }, { "Feb", 2 }, { "Mar", 3 }, { "Apr", 4 },  { "May", 5 },  { "Jun", 6 },
			                { "Jul", 7 }, { "Ago", 8 }, { "Sep", 9 }, { "Oct", 10 }, { "Nov", 11 }, { "Dec", 12 } };

		int thisyear = Date::now().year();

		for (auto& line : lines)
		{
			DirEntry item;
			auto     parts = line.split();
			int      n = parts.length();
			if (n > 3 && parts[0].contains('-') && parts[1].contains(':')) // Windows format?
			{
				auto dmy = parts[0].split('-');
				auto hm = parts[1].split(':');
				int  h = hm[0];
				int  m = hm[1].substr(0, 2);
				if (hm[1].substr(2, 2) == "PM")
					h += 12;
				else if (h == 12)
					h -= 12;
				item.name = line.substring(39);
				item.date = Date(2000 + int(dmy[2]), dmy[0], dmy[1], h, m);
				item.isdir = parts[2] == "<DIR>";
				item.size = item.isdir ? 0 : (int)parts[2];
				items << item;
				continue;
			}

			if (n < 9)
				continue;

			int namei = line.indexOf(" " + parts[7] + " ") + parts[7].length() + 2;
			item.name = line.substr(namei);
			item.isdir = parts[0].startsWith('d');
			item.size = parts[4];
			bool hastime = parts[7].contains(':');
			int  year = hastime ? thisyear : int(parts[7]);

			if (hastime)
			{
				auto hhmm = parts[n - 2].split(':');
				item.date = Date(Date::UTC, year, months[parts[5]], parts[6], hhmm[0], hhmm[1]);
			}
			else
				item.date = Date(year, months[parts[5]], parts[6]);

			items << item;
		}
	}

	endPassive();

	return items;
}

DirEntry FtpClient::info(const String& file)
{
	auto ans = sendCommand("MLST " + file);
	if (!ansOK(ans.first))
		return DirEntry();
	return parseDirEntry(ans.second);
}

ByteArray FtpClient::get(const String& name)
{
	ByteArray data;

	if (!startPassive())
		return data;

	auto ans = sendCommand("RETR " + name);

	if (ans.first > 299)
		return data;

	while (_socketData.connected())
		data.append(_socketData.read(2000));

	endPassive();

	return data;
}

bool FtpClient::put(const String& name, const ByteArray& data)
{
	if (!startPassive())
		return false;

	auto ans = sendCommand("STOR " + name);

	if (ans.first > 299)
		return false;

	int n = _socketData.write(data);

	_socketData.close();

	return endPassive() && n == data.length();
}

bool FtpClient::del(const String& name)
{
	auto ans = sendCommand("DELE " + name);
	return ansOK(ans.first);
}

bool FtpClient::download(const String& name, const String& dest)
{
	if (!startPassive())
		return false;

	String path = (dest == "." || File(dest).isDirectory()) ? (dest + "/" + Path(name).name()) : dest;

	File file(path, File::WRITE);

	auto ans = sendCommand("RETR " + name);
	if (ans.first > 299)
		return false;

	while (_socketData.connected())
		file << _socketData.read(2000);

	endPassive();

	return true;
}

bool FtpClient::upload(const String& name)
{
	if (!startPassive())
		return false;

	auto ans = sendCommand("STOR " + Path(name).name());

	if (ans.first > 299)
		return false;

	File file(name, File::READ);
	if (!file)
		return false;

	int  totali = 0, totalo = 0;
	byte buffer[16000];

	while (!file.end())
	{
		int n = file.read(buffer, sizeof(buffer));
		int m = _socketData.write(buffer, n);
		totali += n;
		totalo += m;
	}

	_socketData.close();

	return endPassive() && totali == totalo;
}

Pair<int, String> FtpClient::sendCommand(const String& cmd)
{
	_socket << cmd + "\r\n";
	return getReply();
}

Pair<int, String> FtpClient::getReply()
{
	Pair<int, String> reply{ 0, String() };

	if (!_socket)
		return reply;

	String ans = _socket.readLine();

	if (ans.endsWith('\r'))
		ans.resize(ans.length() - 1);

	if (ans.length() < 4)
		return reply;

	reply.first = (int)ans.substr(0, 3);
	reply.second = ans.substr(4);
	if (ans[3] == '-')
	{
		while (1)
		{
			ans = _socket.readLine();
			if (ans.endsWith('\r'))
				ans.resize(ans.length() - 1);
			if ((ans.length() > 3 && (int)ans.substr(0, 3) == reply.first) || _socket.disconnected())
				break;
			reply.second << '\n' << ans;
		}
	}
	return reply;
}

bool FtpClient::startPassive()
{
	if (!_socket)
		return false;

	auto ans = sendCommand("PASV");
	if (ans.first != 227)
		return false;

	int i1 = ans.second.indexOf('(');
	int i2 = ans.second.indexOf(')', i1 + 1);

	if (i1 < 0 || i2 < 0)
		return false;
	auto parts = ans.second.substring(i1 + 1, i2).split(',');

	if (parts.length() != 6)
		return false;
	int port = (int(parts[4]) << 8) | int(parts[5]);

	_socketData = Socket();
	return _socketData.connect(_host, port);
}

bool FtpClient::endPassive()
{
	_socketData.close();

	return ansOK(getReply().first);
}
}
