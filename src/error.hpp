#ifndef LUABIND_ERROR_HPP_INCLUDED
#define LUABIND_ERROR_HPP_INCLUDED LUABIND_ERROR_HPP_INCLUDED

#include <boost/exception/exception.hpp>
#include <boost/exception/error_info.hpp>
#include <exception>
#include <lua.hpp>

namespace cpplua {

namespace errinfo {
typedef boost::error_info<struct tag_LuaError, std::string> LuaMsg;
typedef boost::error_info<struct tag_ErrorMessage, std::string> Msg;
typedef boost::error_info<struct tag_ErrorStackIndex, int> StackIndex;
typedef boost::error_info<struct tag_ErrorLuaState, lua_State*> LuaState;
} // namespace error_info

struct Error: virtual std::exception, public boost::exception { };
struct ConversionError: virtual Error { };
struct ToLuaConversionError: virtual ConversionError { };
struct ToCppConversionError: virtual ConversionError { };

} // namespace cpplua

#endif // LUABIND_ERROR_HPP_INCLUDED
