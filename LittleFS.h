#pragma once

#include <FS.h>

class LittleFSClass : public fs::FS
{
};

inline LittleFSClass LittleFS;
