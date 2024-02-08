#include "StorageBase.h"

bool StorageBase::isSafeFilename (const char* filename) {
  int i=0;
  while ((isalnum(filename[i]) || filename[i] == '/') && i < strlen(filename)) i++;

  return i == strlen(filename);
}

void StorageBase::logConsole(String msg) {
  if (STORAGE_LOG_ENABLED) {
    Serial.println(msg.c_str());
  }
}
void StorageBase::logConsole(const char* msg) {
  if (STORAGE_LOG_ENABLED) {
    Serial.println(msg);
  }
}