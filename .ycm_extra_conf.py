def Settings( **kwargs ):
  return {
    'flags': ['-x', 'c++', '-Wall', '-pedantic', '-Isrc', '-Ibitscpp/include',
    '-Iinclude', '-IuSockets/src/libusockets.h',
    '-IuSockets/src',
    '-IGameNetworkingSockets/include', '-std=c++20', '-I/usr/include'],
  }
