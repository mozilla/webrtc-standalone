#ifndef JSON_DOT_H
#define JSON_DOT_H

#include <string>

class JSONParser {
public:
  JSONParser(const char* data);
  ~JSONParser();

  bool isValid(std::string& error);
  bool find(const std::string& key, std::string& value);
  bool find(const std::string& key, int& value);

protected:
  struct State;
  State* mState;
};

class JSONGenerator {
public:
  JSONGenerator();
  ~JSONGenerator();

  bool openMap();
  bool closeMap();
  bool addPair(const std::string& key, const std::string& value);
  bool addPair(const char* key, const std::string& value)
  {
    const std::string strKey(key);
    return addPair(strKey, value);
  }
  bool addPair(const std::string& key, const int value);
  bool addPair(const char* key, const int value)
  {
    const std::string strKey(key);
    return addPair(strKey, value);
  }
  bool getJSON(std::string& key);

protected:
  struct State;
  State* mState;
};

#endif // #define JSON_DOT_H
