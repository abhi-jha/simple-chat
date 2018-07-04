#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include "utils.h"
#include "server.h"

using namespace std;

Server::Server(int port) {
  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(port);
  fPort = port;
  if (bind(fSockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
    debugf("ERROR on binding: %s", strerror(errno));
    fReady = false;
  } else {
    listen(fSockfd, MAX_CLIENTS);
    printf("Server listening on port %d\n", port);
  }
}

Server::~Server() {
  this->closeAll();
}

int Server::readAll(void (*onRead)(int, header, const void*)) {
  int total = 0;
  for (int i = 0; i <= fMaxfd; ++i) {
    if (FD_ISSET(i, &fMasterSet)) {
      total += this->readDataFromFd(i, onRead);
    }
  }
  return total;
}
int Server::writeAll(header h, void* data) {
  int total = 0;
  for (int i = 0; i <= fMaxfd; ++i) {
    if (FD_ISSET(i, &fMasterSet)) {
      total += this->writeDataToFd(i, h, data);
    }
  }
  return total;
}

int Server::acceptConnections() {
  if (!fReady)
    return -1;

  int newfd = 0;
  fd_set workingSet;
  FD_ZERO(&workingSet);
  FD_SET(fSockfd, &workingSet);

  timeval timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = 0;
  int sel = select(fSockfd + 1, &workingSet, NULL, NULL, &timeout);
  if (sel < 0) {
    debugf("select() failed with error %s", strerror(errno));
    return -1;
  } else if (sel == 0) {
    // debugf("select() timed out");
    return -1;
  } else {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    newfd = accept(fSockfd, (struct sockaddr*)&clientAddr, &clientLen);
    if (newfd< 0) {
      debugf("accept() failed with error %s", strerror(errno));
      return -1;
    }
    debugf("New incoming connection - %d", newfd);

    fConnected = true;
    this->setNonBlocking(newfd);
    this->addToMasterSet(newfd);
  }

  return newfd;
}
