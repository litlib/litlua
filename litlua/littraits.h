/********************************************************************

Filename:   littraits.h

Description:littraits.h

Version:  1.0
Created:  20:8:2015   17:27
Revison:  none
Compiler: gcc vc

Author:   wufan, love19862003@163.com

Organization:
*********************************************************************/
#ifndef __lit_traits_h__
#define __lit_traits_h__

#include <new>
#include <stdint.h>
#include <string.h>
#include <lua.hpp>
#include <stdio.h>
#include <typeinfo>
#include <memory>
#include <assert.h>
#include <tuple>
#include <functional>
#include "littype.h"


namespace LitSpace {
	template <std::size_t... M>
	struct _indices {};

	template <std::size_t N, std::size_t... M>
	struct _indices_builder : _indices_builder<N - 1, N - 1, M...> {};

	template <std::size_t... M>
	struct _indices_builder<0, M...> {
		using type = _indices<M...>;
	};

  template<typename F>
  struct guard{
    typedef typename F Fun;
    explicit guard(const Fun& f) : _fun(f){}
    ~guard(){ _fun(); }
    Fun _fun;
  };
  typedef guard<std::function<void()>> guardfun;

 
  template<typename T>
  struct is_table { static constexpr bool ISTABLE = std::is_same<T, table>::value; };

  //��ȡջ����������,��Ҫ��ʵ�ֶ�ȡC�������ò�����lua����ֵ
  template<size_t N, size_t M, typename T>
  struct tuple_reader{
    static constexpr size_t I = M - N;
    static const T& reader(lua_State* L, T& r, int& index){
      auto& ref = std::get<I>(r);
      ref = read<std::remove_reference<decltype(ref)>::type>(L, index);
      tuple_reader< N - 1, M, T>::reader(L, r, ++index);
      return r;
    }
    static bool check_reader(lua_State* L, T& r, int& index, std::string& _error_msg){
      auto& ref = std::get<I>(r);
      bool r1 = check<std::remove_reference<decltype(ref)>::type>(L, index);
      if(r1){
        ref = read<std::remove_reference<decltype(ref)>::type>(L, index);
	  }
	  else {
		  _error_msg.append("\nerror read index:").append(std::to_string(I)).append(" with type:").append(typeid(std::remove_reference<decltype(ref)>::type).name());
	  }
      bool r2 = tuple_reader< N - 1, M, T>::check_reader(L, r, ++index, _error_msg);
      return r2 && r1;
    }

  };

  template<size_t M, typename T>
  struct tuple_reader<1, M, T>{
    static constexpr size_t I = M - 1;
    static const T& reader(lua_State* L, T& r, int& index){
      auto& ref = std::get<I>(r);
      ref = read<std::remove_reference<decltype(ref)>::type>(L, index);
      return r;
    }

    static bool check_reader(lua_State* L, T& r, int& index, std::string& _error_msg){
      auto& ref = std::get<I>(r);
	  bool r1 = check<std::remove_reference<decltype(ref)>::type>(L, index);
	  if (r1) { ref = read<std::remove_reference<decltype(ref)>::type>(L, index); }
	  else { _error_msg.append("\nerror read index:").append(std::to_string(I)).append(" with type:").append(typeid(std::remove_reference<decltype(ref)>::type).name()); }
      return r1;
    }
  };

  //lua��������б����
  template<typename ... ARGS>
  struct lua_args : public std::tuple<  ARGS ...>{
    static constexpr size_t NSIZE = sizeof...(ARGS);
    typedef lua_args args;
    static args reader(lua_State* L, int& index){ args r;return tuple_reader<NSIZE, NSIZE, lua_args>::reader(L, r, index);}
  };

  template<>
  struct lua_args<> : public std::tuple<>{
    static constexpr size_t NSIZE = 0;
    typedef lua_args args;
    static args reader(lua_State* L, int& index){ return args();}
  };

  //luaִ�н������
  template<typename T, typename ... ARGS>
  struct lua_returns : public std::tuple<T,  ARGS ...>{
    static constexpr size_t NSIZE =  1 + sizeof...(ARGS);
    static constexpr bool IS_RETURN = true;
    typedef lua_returns args;
    static args reader(lua_State* L, int& index){
      args r; 
      r._err = !tuple_reader<NSIZE, NSIZE, lua_returns>::check_reader(L, r, index, r._error_msg);
      return r;
    }
    bool _err = false;
    bool _has_table = false;
    std::string _error_msg = "";

    T& get(){
      return std::get<0>(*this);
    }

  };

  template<>
  struct lua_returns<void> :public std::tuple<> {
	  static constexpr size_t NSIZE = 0;
	  static constexpr bool IS_RETURN = true;
	  static constexpr bool CLEAR_STACK = true;
	  typedef lua_returns<void> args;
	  static args reader(lua_State* L, int& index) {
		  return lua_returns<void>();
	  }
	  bool _err = false;
	  bool _has_table = false;
	  std::string _error_msg = "";

	  void get() {

	  }
  };


   //ȫ�ֺ����Ķ����չ��
	template <typename Ret, typename... ARGS, std::size_t... N>
	inline Ret litFunciton(const std::function<Ret(ARGS...)>& func, const std::tuple<ARGS...>& args, _indices<N...>) {
		return func(std::get<N>(args)...);
	}

	template <typename Ret, typename... ARGS>
	inline Ret litFunciton(const std::function<Ret(ARGS...)>& func, const std::tuple<ARGS...>& args) {
		return litFunciton(func, args, typename _indices_builder<sizeof...(ARGS)>::type());
	}

	//���Ա�����Ķ����չ��
	template <typename Ret, typename T, typename... ARGS, std::size_t... N>
	inline Ret classLitFunciton(Ret(T::*func)(ARGS ...), T* t, const std::tuple<ARGS...>& args, _indices<N...>) {
		return (t->*func)(std::get<N>(args)...);
	}

	template <typename Ret, typename T, typename... ARGS>
	inline Ret classLitFunciton(Ret(T::*func)(ARGS ...), T* t, const std::tuple<ARGS...>& args) {
		return classLitFunciton(func, t, args, typename _indices_builder<sizeof...(ARGS)>::type());
	}

	//�๹�캯�������չ��
	template <typename T, typename... ARGS, std::size_t... N>
	inline T* constructorFun(void* p, const std::tuple<ARGS...>& args, _indices<N...>) {
		return new(p)T(std::get<N>(args)...);
	}

	template <typename T>
	inline T* constructorFun(void* p, const std::tuple<>& args) {
		return new(p)T();
	}

	template <typename T, typename... ARGS>
	inline T* constructorFun(void* p, const std::tuple<ARGS...>& args) {
		return constructorFun<T>(p, args, typename _indices_builder<sizeof...(ARGS)>::type());
	}

  inline void push_args(lua_State *L){}

  //���ò�����ջ
  template<typename T>
  inline void push_args(lua_State *L, T t){
    push(L, t);
  }

  template<typename T, typename ... Args>
  inline void push_args(lua_State *L, T t, Args... args){
    push(L, t);
    push_args(L, args...);
  }

  //ȫ�ֺ�����std::function����
  template<typename ... ARGS>
  struct functor : public lua_args< ARGS ...>{
    template<typename Ret>
    static int invoke_func(lua_State* L){
      const std::function<Ret(ARGS...)>& func = upvalue_<std::function<Ret(ARGS...)>>(L);
      if (!check(L, func)) return 0;
      int index = 1;
      push<Ret>(L, litFunciton(func, reader(L, index)));
      return 1;
    }
    template<>
    static int invoke_func<void>(lua_State* L){
      const std::function<void(ARGS...)>& func = upvalue_< std::function<void(ARGS...)>>(L);
      if(!check(L, func)) return 0;
      int index = 1;
      litFunciton(func, reader(L, index));
      return 0;
    }

    template<typename Ret>
    static int invoke(lua_State* L){
      const std::function<Ret(ARGS...)>& func = upvalue_<Ret(*)(ARGS ...)>(L);
      if (!check(L, func))  return 0;
      int index = 1;
      push<Ret>(L, litFunciton(func, reader(L, index)));
      return 1;
    }
    template<>
    static int invoke<void>(lua_State* L){
      const std::function<void(ARGS...)>& func = upvalue_<void(*)(ARGS ...)>(L);
      if (!check(L, func))  return 0;
      int index = 1;
      litFunciton(func, reader(L, index));
      return 0;
    }

    template<typename Ret>
    static bool check(lua_State* L, const std::function<Ret(ARGS...)>& func){
      if (nullptr == func){
        push(L, "lua error with call invalid std::function (nullptr)");
        on_error(L);
        return false;
      }
      return true;
    }
  };

  //���Ա��������
  template< typename T, typename ... ARGS>
  struct classfunctor : public lua_args< ARGS ...>{
    template<typename Ret>
    static int invoke(lua_State* L){
      auto func = upvalue_<Ret(T::*)(ARGS ...)>(L);
      int index = 1;
      T* t = read<T*>(L, index++);
      if(!check(L, t)) return 0;
      push<Ret>(L, classLitFunciton(func, t, reader(L, index)));
      return 1;
    }
    template<>
    static int invoke<void>(lua_State* L){
      auto func = upvalue_<void(T::*)(ARGS ...)>(L);
      int index = 1;
      T* t = read<T*>(L, index++);
      if (!check(L, t)) return 0;
      classLitFunciton(func, t, reader(L, index));
      return 0;
    }

    static bool check(lua_State* L, T* t){
      if (nullptr == t){
        push(L, "lua error with call invalid object(nullptr) member function");
        on_error(L);
        return false;
      }
      return true;
    }
  };


  //����������ز���
  template<typename T>
  struct int_action{
    static constexpr bool value = std::is_integral<T>::value;
    static T invoke(lua_State *L, int index){
      return (T)lua_tointeger(L, index);
    }
    static bool check(lua_State *L, int index){
      return lua_isinteger(L, index) || lua_isnumber(L, index) || lua_isboolean(L, index);
    }

    static void push(lua_State *L, T val){
      lua_pushinteger(L, val);
    }


  };
  template<>
  struct int_action<bool>{
    static constexpr bool value = true;
    static bool invoke(lua_State *L, int index){
      return (bool)lua_toboolean(L, index);
    }
    static bool check(lua_State *L, int index){
      return true;
    }
    static void push(lua_State *L, bool val){
      lua_pushboolean(L, val);
    }
  };

  //������������ز���
  template<typename T>
  struct number_action{
    static constexpr bool value = std::is_floating_point<T>::value;
    static T invoke(lua_State *L, int index){
      return (T)lua_tonumber(L, index);
    }
    static bool check(lua_State *L, int index){
      return lua_isinteger(L, index) || lua_isnumber(L, index) || lua_isboolean(L, index);
    }
    static void push(lua_State *L, T val){
      lua_pushnumber(L, val);
    }
  };

  //�ַ���������ز���
  template<typename T>
  struct str_action{
    static constexpr bool value = std::is_same<T, char*>::value || std::is_same<T, const char*>::value;
    static T invoke(lua_State *L, int index){
      return (T)lua_tostring(L, index);
    }
    static bool check(lua_State *L, int index){
      return lua_isinteger(L, index) || lua_isnumber(L, index) || lua_isboolean(L, index) || lua_isstring(L, index);
    }
    static void push(lua_State *L, T val){
      lua_pushstring(L, val);
    }
  };

  template<>
  struct str_action<std::string>{
    static constexpr bool value = true;
    static std::string invoke(lua_State *L, int index){
      size_t len = 0;
      const char* buffer = lua_tolstring(L, index, &len);
      return std::string(buffer, len);
    }
    static bool check(lua_State *L, int index){
      return lua_isinteger(L, index) || lua_isnumber(L, index) || lua_isboolean(L, index) || lua_isstring(L, index);
    }
    static void push(lua_State *L, std::string val){
      lua_pushlstring(L, val.c_str(), val.length());
    }
  };



  //��Ա����
  struct var_base{
    virtual ~var_base(){};
    virtual void get(lua_State *L) = 0;
    virtual void set(lua_State *L) = 0;
  };


  template<typename T, typename V>
  struct mem_var : var_base{
    V T::*_var;
    mem_var(V T::*val): _var(val){}
    void get(lua_State *L){
      T* t = read<T*>(L, 1);
      if (!check(L, t)) return;
      push<typename std::conditional<is_obj<V>::value, V&, V>::type>(L, t->*(_var)); 
    }
    void set(lua_State *L){ 
      T* t = read<T*>(L, 1);
      if(!check(L, t)) return ;
      T->*(_var) = read<V>(L, 3); 
    }

    bool check(lua_State *L, T* t){
      if (nullptr == t){
        push(L, "lua error with call member var with nullptr");
        on_error(L);
        return false;
      }
      return true;
    }
  };

	//��ȡԭʼ����
 	template<typename A>
 	struct base_type { //std::remove_cvref
 		typedef  typename std::remove_cv<typename std::remove_pointer< typename std::remove_reference<A>::type >::type >::type  type;
 	};

	//��class����
	template<typename A>
	struct class_type {	 
		typedef typename base_type<A>::type type;
	};	  

	//��class����
	template<typename T>
	struct class_name {
		static const char* name(const char* name = NULL) {
			static char temp[256] = "";
			if (name != NULL) { strncpy(temp, name, sizeof(temp) - 1);}
			return temp;
		}
	};

	//�ж��Ƿ��Ƕ�������
	template<typename A>
	struct is_obj { 
		static constexpr bool value = !std::is_integral<A>::value  && !std::is_floating_point<A>::value;// true;
	};
	
	template<> struct is_obj<char*> { static constexpr bool value = false; };
	template<> struct is_obj<const char*> { static constexpr bool value = false; };
	template<> struct is_obj<lua_value*> { static constexpr bool value = false; };
	template<> struct is_obj<table> { static constexpr bool value = false; };
	template<> struct is_obj<nil> { static constexpr bool value = false; };
	template<> struct is_obj<std::string> { static constexpr bool value = false; };

	//void*ָ��ת����Ӧ���ͷ���
	template<typename T>
	struct void2val { 
		static T invoke(void* input) {return *(T*)input;} 
	};
	template<typename T>
	struct void2ptr { 
    static T* invoke(void* input){return (T*)input;} 
	};
	template<typename T>
	struct void2ref { 
		static T& invoke(void* input) {return *(T*)input; } 
	};


	template<typename T>
	struct void2type
	{
		static T invoke(void* ptr)
		{
			return  std::conditional<std::is_pointer<T>::value
				, void2ptr<typename base_type<T>::type>
				, typename std::conditional<std::is_reference<T>::value
				, void2ref<typename base_type<T>::type>
				, void2val<typename base_type<T>::type>
				>::type
			>::type::invoke(ptr);
		}
	};

	//c++����ĵ�ַ�� ptr ref value ����һ����ַ
	struct user{
		user(void* p) : m_p(p){}
		virtual ~user() {}
		void* m_p;
	};

	template<typename T>
	struct user2type { 
		static T invoke(lua_State *L, int index) {
			return void2type<T>::invoke(lua_touserdata(L, index)); 
		} 
	};

	//lua��ö�ٵķ���
	template<typename T>
		struct lua2enum {
			static T invoke(lua_State *L, int index) { 
				return (T)(int)lua_tonumber(L, index);
			} 
		static bool check(lua_State *L, int index){
		  return lua_isinteger(L, index);
		}
	};

	//lua������ķ��䣬���Ƿ���ֵ��ʱ�򣬻�ȥ���ö����check����Ϊ������ʱ��ֱ�Ӵ���
	template<typename T>
	struct lua2object{
		static T invoke(lua_State *L, int index){
			bool err = !lua_isuserdata(L, index);
			if (err){
				auto t =  T();
				std::string note;
				note.append( "\ntype:[").append(typeid(t).name()).append("]call class type error \n1.first argument. (forgot ':' expression ?)");
				push(L, note);
				err = true;
				lua_error(L);
			}
			return void2type<T>::invoke(user2type<user*>::invoke(L, index)->m_p);
		}

		static bool check(lua_State *L, int index){
		  return lua_isuserdata(L, index);
		}
	};

	//luaת������Ӧ�������� nil tableͨ��read��ƫ�ػ�ʵ��
	template<typename T>
	T lua2type(lua_State *L, int index){
		return std::conditional< int_action<T>::value, int_action<T>,
		  typename std::conditional< number_action<T>::value, number_action<T>,
		  typename std::conditional< str_action<T>::value, str_action<T>,
		  typename std::conditional<std::is_enum<T>::value, lua2enum<T>, lua2object<T>	>::type
		  >::type
		  >::type
		>::type::invoke(L, index);

	}

	//��Ҫ��ⷵ��ֵ�Ƿ�Ϸ�
	//���lua�ܷ�ת������Ӧ���� nil tableͨ��read��ƫ�ػ�ʵ��
	  template<typename T>
	  bool checklua2type(lua_State *L, int index){
		return std::conditional< int_action<T>::value, int_action<T>,
		  typename std::conditional< number_action<T>::value, number_action<T>,
		  typename std::conditional< str_action<T>::value, str_action<T>,
		  typename std::conditional<std::is_enum<T>::value, lua2enum<T>, lua2object<T>	>::type
		  >::type
		  >::type
		>::type::check(L, index);
	  }

	//class���Ͱ󶨶���
	template<typename T>
	struct val2user : user{
		template<typename ... ARGS>
		val2user(ARGS ... args) : user(new T(args...)) {}
		~val2user() { delete ((T*)m_p); }
	};


	template<typename T>
	struct ptr2user : user{
		ptr2user(T* t) : user((void*)t) {}
	};


	template<typename T>
	struct ref2user : user{
		ref2user(T& t) : user(&t) {}
	};

	//class���Ͱ�lua����
	template<typename T>
	struct val2lua {
		static void invoke(lua_State *L, T& input) {
			new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(input); 
		}
	};

	template<typename T>
	struct ptr2lua {
		static void invoke(lua_State *L, T* input) {
			if (input) { new(lua_newuserdata(L, sizeof(ptr2user<T>))) ptr2user<T>(input);}
      else{ lua_pushnil(L); }
		} 
	};

	template<typename T>
	struct ref2lua { 
		static void invoke(lua_State *L, T& input) { 
			new(lua_newuserdata(L, sizeof(ref2user<T>))) ref2user<T>(input); 
		} 
	};

	//ö��תlua
	template<typename T>
	struct enum2lua {
		static void push(lua_State *L, T val) { 
			lua_pushnumber(L, (int)val);
		}
	};

	//class����lua
	template<typename T>
	struct object2lua{
		static void push(lua_State *L, T val){
			std::conditional<std::is_pointer<T>::value
				, ptr2lua<typename base_type<T>::type>
				, typename std::conditional<std::is_reference<T>::value
				, ref2lua<typename base_type<T>::type>
				, val2lua<typename base_type<T>::type>
				>::type
			>::type::invoke(L, val);

			// set metatable
			push_meta(L, class_name<typename class_type<T>::type>::name());
			lua_setmetatable(L, -2);
		}
	};

	//��ջ����
	template<typename T>
	void type2lua(lua_State *L, T val){
		//std::conditional<std::is_enum<T>::value, enum2lua<T>, object2lua<T>>::type::push(L, val);
		return std::conditional< int_action<T>::value, int_action<T>,
		  typename std::conditional< number_action<T>::value, number_action<T>,
		  typename std::conditional< str_action<T>::value, str_action<T>,
		  typename std::conditional<std::is_enum<T>::value, enum2lua<T>, object2lua<T>	>::type
		  >::type
		  >::type
		>::type::push(L, val);
	}

	//��ȡC�����հ�
	template<typename T>
	T upvalue_(lua_State *L){
		return user2type<T>::invoke(L, lua_upvalueindex(1));
	}

	//ȫ�ֺ����հ���ջ
	template<typename R, typename ... ARGS >
	void push_function(lua_State *L, R(*func)(ARGS ...)) {
		(void)func;
		lua_pushcclosure(L, functor<ARGS...>::invoke<R>, 1);
	}


	//std::function�հ���ջ func�����������Ч�ڻ�Ӱ�����
	template<typename R, typename ... ARGS >
	void push_function2(lua_State *L, const std::function<R( ARGS...)>& func) {
		(void)func;
		lua_pushcclosure(L, functor<ARGS...>::invoke_func<R>, 1);
	}


	//��Ա�����հ���ջ
	template<typename R, typename T, typename ... ARGS >
	void push_mfunctor(lua_State *L, R(T::*func)(ARGS ...)) {
		(void)func;
		lua_pushcclosure(L, classfunctor<T, ARGS...>::invoke<R>, 1);
	}

	//���캯����ջ
	template<typename T, typename ... ARGS>
	int constructor(lua_State *L) {
		val2user<T>* p = (val2user<T>*)lua_newuserdata(L, sizeof(val2user<T>));
		int index = 2;
		constructorFun<val2user<T>>(p, lua_args<ARGS ...>::reader(L, index));
		push_meta(L, class_name<typename class_type<T>::type>::name());
		lua_setmetatable(L, -2);
		return 1;
	}

	//��������
	template<typename T>
	int destroyer(lua_State *L) {
		((user*)lua_touserdata(L, 1))->~user();
		return 0;
	}
}

#endif