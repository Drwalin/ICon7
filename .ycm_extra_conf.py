def Settings( **kwargs ):
  return {
    'flags': ['-x', 'c++', '-Wall', '-pedantic', '-Isrc', '-Ibitscpp/include',
    '-Iinclude', '-IuSockets/src/libusockets.h', '-Irpmalloc/rpmalloc',
    '-IuSockets/src', '-std=c++23', '-I/usr/include',
    '-DBITSCPP_BYTE_WRITER_V2_BT_TYPE=icon7::ByteBufferWritable',
    '-DBITSCPP_BYTE_WRITER_V2_NAME_SUFFIX=_ByteBuffer'],
  }
