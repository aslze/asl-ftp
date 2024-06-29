// Copyright 2024 ASL author
// Licensed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ASL_FTP_H
#define ASL_FTP_H

#include <asl/String.h>
#include <asl/Socket.h>
#include <asl/Date.h>

namespace asl
{
struct DirEntry
{
	String name;
	bool   isdir;
	Long   size;
	Date   date;
};

/**
A basic FTP client
*/
class FtpClient
{
public:
	FtpClient();

	/**
	 * Connects to an FTP server at a given host and port (default 21), and returns true on success
	 */
	bool connect(const String& host, int port = 21);
	
	/**
	 * Logs in to a connected server with a user an password (by default uses anonymous login) and returns true on success
	 */
	bool login(const String& user = "", const String& pass = "");
	
	/**
	 * Disconnects from a server
	 */
	void disconnect();
	/**
	 * Changes the remote current directory
	 */
	bool cd(const String& dir);

	/**
	 * Gets a list of files and directories at a given remote path, each with name, size and modification date
	 */
	Array<DirEntry> list(const String& dir);

	/**
	 * Gets information on a single file or directory (only available in servers supporting MLST)
	 */
	DirEntry info(const String& file);

	/**
	 * Gets the content of a file as a byte array (without storing to a file)
	 */
	ByteArray get(const String& name);
	
	/**
	 * Puts a new remote file with content from a byte array
	 */
	bool put(const String& name, const ByteArray& data);
	
	/**
	 * Downloads a remote file to a local file (dest is a directory or a file path, default is current dir)
	 */
	bool download(const String& name, const String& dest = ".");
	
	/**
	 * Uploads a local file to the server at the remote current directory
	 */
	bool upload(const String& name);
	
	/**
	 * Deletes a remote file
	 */
	bool del(const String& name);

	Pair<int, String> sendCommand(const String& cmd);

protected:
	Pair<int, String> getReply();
	bool              startPassive();
	bool              endPassive();

private:
	String _host;
	Socket _socket;
	Socket _socketData;
	bool   _hasMlsd;
};

}

#endif
