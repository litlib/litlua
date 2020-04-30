/********************************************************************

Filename:   littype.h

Description:littype.h

Version:  1.0
Created:  25:11:2018   17:27
Revison:  none
Compiler: gcc vc

Author:   wufan, love19862003@163.com

Organization:
*********************************************************************/
#ifndef __lit_type_h__
#define __lit_type_h__

#include <memory>
#include <assert.h>
#include <string>
#include <iostream>
#include "lua.hpp"

namespace LitSpace{

  //�������Ƿ���ת���ɶ�Ӧ����ֵ
  template<typename T>
  inline bool check(lua_State* L, int index){
    return checklua2type<T>(L, index);
  }

  //��ȡ����
  template<typename T>
  inline T read(lua_State* L, int index){
    return lua2type<T>(L, index);
  }

  //��ջ����
  template<typename T>
  inline void push(lua_State *L, T ret){
    type2lua<T>(L, ret); 
  }

 
  template<typename T>
  inline T pop(lua_State *L){ T t = read<T>(L, -1); lua_pop(L, 1); return t; }

  template<>
  inline void  pop(lua_State *L){ lua_pop(L, 1); }
  struct lua_value{
    virtual void to_lua(lua_State *L) = 0;
  };

  //nil ����
  struct nil{
    nil():m_nil(true){}
    nil(bool n):m_nil(n){}
    bool m_nil = true;
  };

  //table����
  struct table_impl{
    table_impl(lua_State* L, int index): m_L(L), m_index(index), m_p(nullptr){
      if (lua_isnil(m_L, m_index)){
        lua_remove(m_L, m_index);
      } else{
        m_p = lua_topointer(m_L, m_index);
      }
    }
    virtual ~table_impl(){
      if (validate()){ lua_remove(m_L, m_index); }
    }

    bool validate(){
      if (nullptr == m_p) return false;
      if (lua_topointer(m_L, m_index) == m_p) return true;

      for (int i = 1; i <= lua_gettop(m_L); ++i){
        if (lua_topointer(m_L, i) == m_p){
          m_index = i;
          return true;
        }
      }

      m_p = nullptr;
      return false;
    }

    template<typename T>
    void set(const std::string& name, T object){
      if (validate()){
        lua_pushlstring(m_L, name.data(), name.length());
        push(m_L, object);
        lua_settable(m_L, m_index);
      }
    }

    template<typename T>
    bool add(T object){
      if (validate()){
        unsigned int len = length();
        lua_pushinteger(m_L, len + 1);
        push(m_L, object);
        lua_settable(m_L, m_index);
        return true;
      }
      return false;
    }

    bool has(int index){
      if (validate()){
        lua_pushinteger(m_L, index);
        lua_gettable(m_L, m_index);
        return !pop<nil>(m_L).m_nil;
      }
      return false;
    }

    bool has(const char* name){
      if (validate()){
        lua_pushstring(m_L, name);
        lua_gettable(m_L, m_index);
        return !pop<nil>(m_L).m_nil;
      } else{
        return false;
      }
    }


    unsigned int length(){
      if (validate()){
        lua_len(m_L, m_index);
      } else{
        return 0;
      }
      return pop<unsigned int>(m_L);
    }


    template<typename T>
    T get(int index){
      if (validate()){
        lua_pushinteger(m_L, index);
        lua_gettable(m_L, m_index);
      } else{
        lua_pushnil(m_L);
      }
      return pop<T>(m_L);
    }

    

    template<typename T>
    T get(const std::string& name){
      if (validate()){
        lua_pushstring(m_L, name.data());
        lua_gettable(m_L, m_index);
      } else{
        lua_pushnil(m_L);
      }
      return pop<T>(m_L);
    }

	//�������,���ջ״̬����ɵ���ǰ�Ķ���
    template<typename lua_returns, typename ... ARGS>
    lua_returns rcall(const char* name, ARGS ... args){
      if (validate()){
        int index = lua_gettop(m_L);
        int nresult = lua_returns::NSIZE;
        lua_pushcclosure(m_L, on_error, 0);
        guardfun g([index, this](){
          lua_settop(m_L, index);
        });
        int errfunc = lua_gettop(m_L);
        lua_pushstring(m_L, name);
        lua_gettable(m_L, m_index);

        if (lua_isfunction(m_L, -1)){
          push_args(m_L, args ...);
          lua_pcall(m_L, sizeof ...(ARGS), nresult, errfunc);
        } else{
          print_error(m_L, "litlua table call() attempt to call table function `%s' (not a function)", name);
        }
        int readindex = 0 - nresult;
        return lua_returns::reader(m_L, readindex);
  
      } else{
        lua_pushnil(m_L);
      }
      lua_returns e;
      e._err = true;
      return e; 
    }

    void debug(){
      lua_pushnil(m_L); 
      while (lua_next(m_L, m_index) != 0){
        if (lua_isnumber(m_L, -2))
          std::cout << "key:" << lua_tonumber(m_L, -2) << '\t';
        else if (lua_isstring(m_L, -2))
          std::cout << "key:" << lua_tostring(m_L, -2) << '\t';
        if (lua_isnumber(m_L, -1))
          std::cout << "value:" << lua_tonumber(m_L, -1) << std::endl;
        else if (lua_isstring(m_L, -1))
          std::cout << "value:" << lua_tostring(m_L, -1) << std::endl;
        else if (lua_istable(m_L, -1))
          std::cout << "value : table:" << lua_topointer(m_L, -1) << std::endl;

        //printf("%s - %s\n",
         //      lua_typename(m_L, lua_type(m_L, -2)),
        //       lua_typename(m_L, lua_type(m_L, -1)));
        lua_pop(m_L, 1);
      }
      //lua_pop(m_L, 1);
    }

    lua_State* state(){ return m_L; }


    lua_State* m_L;
    int m_index;
    const void* m_p;
  };


  //lua table��أ����뱣֤table��ջ�ϣ��������Ч
  struct table{
    table():m_nil(true), m_obj(nullptr){}

    table(lua_State* L){
      lua_newtable(L);
      m_obj = std::make_shared<table_impl>(L, lua_gettop(L));
    }
    table(lua_State* L, const char* name):m_nil(false){
      lua_getglobal(L, name);
      if (lua_istable(L, -1)){
        lua_pop(L, 1);
        lua_newtable(L);
        lua_setglobal(L, name);
        lua_getglobal(L, name);
      }

      m_obj = std::make_shared<table_impl>(L, lua_gettop(L));
    }
    table(lua_State* L, int index):m_nil(false){
      if (index < 0){
        index = lua_gettop(L) + index + 1;
      }
      m_obj = std::make_shared<table_impl>(L, index);
    }
    table(const table& input):m_nil(input.m_nil), m_obj(input.m_obj){

    };

    void debug(){
      if (!isNil()){
        m_obj->debug();
      }
      
    }

	//��ӳ�Ա k=v
    template<typename T>
    void set(const char* name, T object){
		  checkNil();
		  if (m_nil){ return; }
		  m_obj->set(name, object);
    }

	//��ӳ�Ա index=v
    template<typename T>
    bool add(T object){
		  checkNil();
		  if (m_nil){   return false; }
		  return m_obj->add(object);
    }

	//��ӳ�Ա k=new table
    table table::child(const char* name){
		  checkNil();
		  if (m_nil){  return nilTable(); }
		  table t(m_obj->state());
		  set(name, t);
		  return std::move(t);
    }

	//��ӳ�Ա index=new table
    table table::child(){
		  checkNil();
		  if (m_nil){  return nilTable(); }
		  table t(m_obj->state());
		  add(t);
		  return std::move(t);
    }

	//�ж��Ƿ��г�Ա if table[k] != nil
    bool table::has(const char* name){
		  checkNil();
		  if (m_nil){  return false; }
		  return m_obj->has(name);
    }

	//�ж��Ƿ��г�Ա if table[index] != nil
    bool table::has(int index){
		  checkNil();
		  if (m_nil){  return false; }
		  return m_obj->has(index);
    }

	//#table
    unsigned int table::len(){
		  checkNil();
		  if (m_nil){  return 0; }
		  return m_obj->length();
    }

	//��ȡ��Ա���� v = table[k]
    template<typename T>
    T get(const char* name){
      checkNil();
      if (m_nil){ return T(); }
      return m_obj->get<T>(name);
    }

	//��ȡ��Ա���� v = table[index]
    template<typename T>
    T get(int index){
		  checkNil();
		  if (m_nil){  return T(); }
		  return m_obj->get<T>(index);
    }


	//�෵��ֵ��Ա�������� lua_returns = table.fun(args...)
    template<typename lua_returns, typename ... ARGS>
    lua_returns rcall(const char* name, ARGS ... obj){
		  checkNil();
		  if (m_nil){  return lua_returns(); }
		  return m_obj->rcall<lua_returns, ARGS ...>(name, obj...);
    }

	
	//������ֵ��Ա�������� R = table.fun(args...)
    template<typename R, typename ... ARGS>
    R call(const char* name, ARGS ... obj){
		  checkNil();
		  if (m_nil) { lua_returns<R> r;  return  r.get(); }
		  auto r = m_obj->rcall<lua_returns<R>, ARGS ...>(name, obj...);
		  return r.get();
    }

	void checkNil() {
 		if (m_obj){
 			m_nil = !m_obj->validate();
 		}
 		else {
 			m_nil = true;
 		}
	}

  bool isNil(){ /*checkNil();*/ return m_nil; }
    
	void reset() { m_nil = true; m_obj.reset(); }

    std::shared_ptr<table_impl>  m_obj;	 //���ö���
    bool  m_nil = false;				 //�Ƿ���nil
    static table nilTable(){ static table t;  return t; }
  };

  
  template<>
  inline table read(lua_State *L, int index){
    return table(L, index);
  }
  template<>
  inline nil read(lua_State *L, int index){
    return nil(lua_isnil(L, index));
  }
  template<>
  inline bool check<table>(lua_State* L, int index){
    return lua_istable(L, index);
  }

  template<>
  inline bool check<nil>(lua_State* L, int index){
    return true;
  }

  template<>
  inline void push<nil>(lua_State *L, nil /*ret*/){
    lua_pushnil(L);
  }

  template<>
  inline void push<table>(lua_State *L, table ret){
    if (!ret.m_nil)
      lua_pushvalue(L, ret.m_obj->m_index);
    else
      lua_pushnil(L);
  }


  template<>
  table table_impl::get<table>(int index){
    if (validate()){
      lua_pushinteger(m_L, index);
      lua_gettable(m_L, m_index);
    } else{
      lua_pushnil(m_L);
    }
    return read<table>(m_L, -1);
  }

  template<>
  table table_impl::get<table>(const std::string& name){
    if (validate()){
      lua_pushstring(m_L, name.data());
      lua_gettable(m_L, m_index);
    } else{
      lua_pushnil(m_L);
    }
    return read<table>(m_L, -1);
  }

}

#endif//