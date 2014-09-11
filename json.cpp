#include "api/yajl_gen.h"
#include "api/yajl_tree.h"
#include "json.h"
#include <sstream>

#define VALIDATE_STATE() if(!mState) { return false; }

struct JSONParser::State {
  yajl_val mTree;
  bool mValid;
  std::string mError;

  State() : mTree(NULL), mValid(true) {};
  ~State()
  {
    yajl_tree_free(mTree); mTree = NULL;
  }
};

JSONParser::JSONParser(const char* data) : mState(NULL)
{
  const int errorSize = 1024;
  char error[errorSize];
  mState = new State;

  mState->mTree = yajl_tree_parse(data, error, errorSize);

  if (!mState->mTree) {
    mState->mValid = false;
    mState->mError = error;
  }
}

JSONParser::~JSONParser()
{
  delete mState; mState = NULL;
}

bool
JSONParser::isValid(std::string& error)
{
  VALIDATE_STATE();
  error = mState->mError;
  return mState->mValid;
}

bool
JSONParser::find(const std::string& key, std::string& value)
{
  VALIDATE_STATE();
  bool result = false;

  if (mState->mTree) {
    const char* path[2] = { key.c_str(), NULL };
    yajl_val str = yajl_tree_get(mState->mTree, path, yajl_t_string);
    if (str) {
      result = true;
      value = YAJL_GET_STRING(str);
    }
  }
  return result;
}

bool
JSONParser::find(const std::string& key, int& value)
{
  VALIDATE_STATE();
  bool result = false;

  if (mState->mTree) {
    const char* path[2] = { key.c_str(), NULL };
    yajl_val num = yajl_tree_get(mState->mTree, path, yajl_t_number);
    if (YAJL_IS_INTEGER(num)) {
      result = true;
      value = (int)YAJL_GET_INTEGER(num);
    }
  }
  return result;
}

struct JSONGenerator::State {
  yajl_gen mGen;
  State() : mGen(yajl_gen_alloc(NULL)) {}
  ~State()
  {
    yajl_gen_free(mGen);
  }
};

JSONGenerator::JSONGenerator() : mState (NULL)
{
  mState = new State;
}

JSONGenerator::~JSONGenerator()
{
  delete mState; mState = NULL;
}

bool
JSONGenerator::openMap()
{
  VALIDATE_STATE();
  if (yajl_gen_map_open(mState->mGen) != yajl_gen_status_ok) {
    return false;
  }
  return true;
}

bool
JSONGenerator::closeMap()
{
  VALIDATE_STATE();
  if (yajl_gen_map_close(mState->mGen) != yajl_gen_status_ok) {
    return false;
  }
  return true;
}

bool
JSONGenerator::addPair(const std::string& key, const std::string& value)
{
  VALIDATE_STATE();
  if (yajl_gen_string(mState->mGen, (const unsigned char *)key.c_str(), key.length()) != yajl_gen_status_ok) {
    return false;
  } else if (yajl_gen_string(mState->mGen, (const unsigned char *)value.c_str(), value.length()) != yajl_gen_status_ok) {
    return false;
  }
  return true;
}

bool
JSONGenerator::addPair(const std::string& key, const int value)
{
  VALIDATE_STATE();
  std::stringstream stream;
  stream << value;
  std::string str = stream.str();
  if (yajl_gen_string(mState->mGen, (const unsigned char *)key.c_str(), key.length()) != yajl_gen_status_ok) {
    return false;
  } else if (yajl_gen_number(mState->mGen, str.c_str(), str.length()) != yajl_gen_status_ok) {
    return false;
  }
  return true;
}

bool
JSONGenerator::getJSON(std::string& key)
{
  VALIDATE_STATE();
  const unsigned char* buf = NULL;
  size_t len = 0;
  if (yajl_gen_get_buf(mState->mGen, &buf, &len) != yajl_gen_status_ok) {
    return false;
  }
  key = (const char*)buf;
  return true;
}

