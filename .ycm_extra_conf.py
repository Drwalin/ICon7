def Settings( **kwargs ):
  return {
    'flags': ['-x', 'c++', '-Wall', '-pedantic', '-Isrc', '-Ibitscpp/include',
    '-Iinclude', '-IuSockets/src/libusockets.h',
    '-IuSockets/src', '-std=c++20', '-I/usr/include'],
  }
