# asl-ftp

A very simple and incomplete FTP client class.

* Only supports *passive* data transfers, and assumes same host for data as for commands
* Only plain FTP (not encrypted) at the momment (maybe implicit TLS in the future)
* Directory listing in older servers not supporting MLSD may give wrong results

Simplified example (most commands will return false on failure):

```cpp
FtpClient ftp;
ftp.connect("ftp.server.net");
ftp.login("me", "pass");

// list content of a directory

auto list = ftp.list("/some/dir");

for (auto& item : items)
{
	if(item.isdir)
		printf("%s <DIR> modified %s\n", *item.name, *item.date.toString());
	else
		printf("%s %lli bytes, modified %s\n", *item.name, item.size, *item.date.toString());
}

// download a file into a buffer

auto data = ftp.get("/some/file");

// download to a local file

ftp.download("/some/file", "/local/dir");

// upload a file to a remote directory

ftp.cd("/pub/images");
ftp.upload("/localdir/image.png");

// delete a file

ftp.del("/pub/oldimage.png");
```

Can use it with CMake like this (including ASL first):

```cmake
include(FetchContent)
FetchContent_Declare(aslftp URL https://github.com/aslze/asl-ftp/archive/1.0.zip)
FetchContent_MakeAvailable(aslftp)

target_link_libraries(myprogram aslftp)
```

