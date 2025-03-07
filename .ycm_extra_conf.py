def Settings( **kwargs ):
  return {
    'flags': ['-x', 'c++', '-Wall', '-pedantic', '-Isrc', '-Ibitscpp/include',
    '-Iinclude', '-IuSockets/src/libusockets.h', '-Irpmalloc/rpmalloc',
    '-IuSockets/src', '-std=c++23', '-I/usr/include'],
  }
