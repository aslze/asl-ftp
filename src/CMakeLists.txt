set(TARGET aslftp)

set(SRC
	FtpClient.cpp
)

set(INC ../include/asl)

set(HEADERS
    ${INC}/FtpClient.h
)

add_library(${TARGET} STATIC ${SRC} ${HEADERS}) 
target_link_libraries(${TARGET} asls) 
target_include_directories(${TARGET} PUBLIC ../include)
