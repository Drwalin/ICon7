def Settings( **kwargs ):
  return {
    'flags': ['-x', 'c++', '-Wall', '-pedantic', '-Isrc', '-Ibitscpp/include',
    '-Iinclude', '-IuSockets/src/libusockets.h',
    '-IGameNetworkingSockets/include', '-std=c++17', '-I/usr/include'],
  }
