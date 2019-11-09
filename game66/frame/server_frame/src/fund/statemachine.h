#ifndef _STATE_MACHINE_H_2003_01_15
#define _STATE_MACHINE_H_2003_01_15
#include <map>
#include <assert.h>
#include "fundutil.h" 
#include "type.h"

namespace fund {
	namespace flowcontrol {
		//FUNC: function type,
		//STATE: state type,
		//PARAM: function's parameter type,
		//MSG: message type
		enum enumFLOWSTATE {
			NORMAL_STATE, END_STATE, ERROR_STATE
		};

		enum enumEXTSTATE{
			EXIST_STATE, NOEXIST_STATE
		};

		enum enumPARAM {
			INSIDE_PARAM, OUTSIDE_PARAM
		};

#ifndef _MSC_VER
		template <enumPARAM enumParam, typename PARAM>
		struct data {
		};

		template <typename PARAM>
		struct data<INSIDE_PARAM, PARAM> {
			PARAM _param;
		};
#endif

		template <typename FUNC, 
				typename STATE, 
				typename PARAM, 
				typename MSG, 
				enumEXTSTATE enumSupport = EXIST_STATE,
				enumPARAM enumParam = INSIDE_PARAM>
		class statemachine {
#ifdef _MSC_VER
			template <enumPARAM enumParam>
			struct data {
			};

			template <>
			struct data<INSIDE_PARAM> {
				PARAM _param;
			};
#endif
		
			class msg {
				enum enumNEXTSTATE {
					NOTHAS_NEXTSTATE, HAS_NEXTSTATE
				};

				enum enumHASFUNC {
					NOTHAS_FUNC, HAS_FUNC
				};
			public:
				msg(const STATE& nextstate) //这里的pFunc是不使用的。
					: _enumHasFunc(NOTHAS_FUNC), 
					_func(0),
					_enumNextState(HAS_NEXTSTATE),
					_nextstate(nextstate) {
				}

				msg(FUNC func) 
					: _enumHasFunc(HAS_FUNC),
					_enumNextState(NOTHAS_NEXTSTATE)
					, _func(new FUNC(func)) {
				}

				msg(FUNC func, const STATE& nextstate) 
					: _enumHasFunc(HAS_FUNC), _func(new FUNC(func)), 
					_enumNextState(HAS_NEXTSTATE), _nextstate(nextstate) {
				}

				msg(const msg& v) 
					:_enumHasFunc(v._enumHasFunc),
					_enumNextState(v._enumNextState),
					_nextstate(v._nextstate) {
						if (v._func) {
							_func = new FUNC(*v._func);
						}
						else {
							_func = 0;
						}
				}

				msg& operator = (const msg& v) {
					if (&v == this) {
						return *this;
					}
					_enumHasFunc = v._enumHasFunc;
					delete _func;
					if (v._func) {
						_func = new FUNC(*v._func);
					}
					else {
						_func = 0;
					}
					_enumNextState = v._enumNextState;
					_nextstate = v._nextstate;
					
					
					return *this;
				}

				virtual ~msg() {
					delete _func;
				}

				void do_it(PARAM& param, STATE& nextstate) {
					do_it(Int2Type<enumSupport>(), param, nextstate);
				}

			private:
				void do_it(Int2Type<NOEXIST_STATE>, PARAM& param, STATE& nextstate) {
					if (_enumNextState == NOTHAS_NEXTSTATE) {
						nextstate = (*_func)(Ref(param));
					}
					else {
						nextstate = _nextstate;
						if (HAS_FUNC == _enumHasFunc) {
							(*_func)(Ref(param));
						}
					}
				}

				void do_it(Int2Type<EXIST_STATE>, PARAM& param, STATE& nextstate) {
					nextstate = _nextstate;
					if (HAS_FUNC == _enumHasFunc) {
						(*_func)(Ref(param));
					}
				}

			private:
				enumHASFUNC _enumHasFunc;
				FUNC* _func;
				enumNEXTSTATE _enumNextState;
				STATE _nextstate;
			};//end class msg

			class state {
			public:
				state() : _defaultmsg(0) {
				}

				virtual ~state() {
					delete _defaultmsg;
				}

				state(const state& rhs)
					: _mapms(rhs._mapms) {
					if (rhs._defaultmsg) {
						_defaultmsg = new msg(*rhs._defaultmsg);
					}
					else {
						_defaultmsg = 0;
					}
				}

				state& operator = (const state& rhs) {
					if (&rhs != this) {
						delete rhs._defaultmsg;
						if (rhs._defaultmsg) {
							_defaultmsg = new msg(*rhs._defaultmsg);
						}
						else {
							_defaultmsg = 0;
						}
					}
					_mapms = rhs._mapms;
					return *this;
				}

				state& add(const MSG& m, const msg& st) {
					assert(_mapms.find(m) == _mapms.end());
					_mapms.insert(MAP_VALUE(MAPMSG, m, st));
					return *this;
				}

				state& add(const msg& st) {
					assert (!_defaultmsg);
					_defaultmsg = new msg(st);
					return *this;
				}

				void do_it(const MSG& m, PARAM& param, STATE& nextstate) {
					typename MAPMSG::iterator it = _mapms.find(m);
					if (it != _mapms.end()) {
						it->second.do_it(param, nextstate);
					}
					else if (_defaultmsg) {
						_defaultmsg->do_it(param, nextstate);
					}
				}

			private:
				typedef std::map<MSG, msg> MAPMSG;
				msg * _defaultmsg;
				MAPMSG _mapms;
			}; //end class state

		public:
			statemachine(const STATE& startstate, const STATE& endstate) 
				:_startstate(startstate), _state(startstate), _endstate(endstate) {
			}

			statemachine(const STATE& startstate, const STATE& endstate, 
						const PARAM& param) 
				:_startstate(startstate), _state(startstate), 
				_endstate(endstate){
				_data._param = param;
			}

			virtual ~statemachine() {
			}

			statemachine(const statemachine& v)
				:_startstate(v._startstate),
				 _endstate(v._endstate),
				 _state(v._state),
				 _mapstate(v._mapstate) {
			}

			statemachine& operator= (const statemachine& v) {
				if (&v == this) {
					return *this;
				}
				_startstate = v._startstate;
				_endstate = v._endstate;
				_state = v._state;
				_mapstate = v._mapstate;
				return *this;
			}

			virtual statemachine* clone() {
				return new statemachine(_startstate, _endstate, _mapstate);
			}

			statemachine& add(const STATE& state, const MSG& m, FUNC func) {
				return add(Int2Type<enumSupport>(), state, m, func);
			}

			statemachine& add(const STATE& state, FUNC func) {
				return add(Int2Type<enumSupport>(), state, func);
			}

			statemachine& add(const STATE& st, const MSG& m, const STATE& nextstate) {
				typename MAP_MSGSTATUS::iterator it = _mapstate.find(st);
				if (it != _mapstate.end()) {
					it->second.Add(m, msg(nextstate));
				}
				else {
					_mapstate.insert(
						MAP_VALUE(MAP_MSGSTATUS, st, 
											state().add(m, msg(nextstate))));
				}
				return *this;
			}

			statemachine& add(const STATE& st, const STATE& nextstate) {
				typename MAP_MSGSTATUS::iterator it = _mapstate.find(st);
				if (it != _mapstate.end()) {
					it->second.Add( msg(nextstate));
				}
				else {
					_mapstate.insert(
						MAP_VALUE(MAP_MSGSTATUS, st, 
											state().add(msg(nextstate))));
				}
				return *this;
			}

			statemachine& add(const STATE& st, const MSG& m, 
							FUNC func, const STATE& nextstate) {
				typename MAP_MSGSTATUS::iterator it = _mapstate.find(st);
				if (it != _mapstate.end()) {
					it->second.Add(m, msg(func, nextstate));
				}
				else {
					_mapstate.insert(
						MAP_VALUE(MAP_MSGSTATUS, st, 
											state().add(m, msg(func, nextstate))));
				}
				return *this;
			}

			statemachine& add(const STATE& st, FUNC func, const STATE& nextstate) {
				typename MAP_MSGSTATUS::iterator it = _mapstate.find(st);
				if (it != _mapstate.end()) {
					it->second.Add(msg(func, nextstate));
				}
				else {
					_mapstate.insert(
						MAP_VALUE(MAP_MSGSTATUS, st, 
											state().add(msg(func, nextstate))));
				}
				return *this;
			}

			enumFLOWSTATE do_it(const MSG& msg, PARAM& param) {
				return do_it(msg, param, Int2Type<enumParam>());
			}

			enumFLOWSTATE do_it(const MSG& msg) {
				return do_it(msg, Int2Type<enumParam>());
			}
			
			STATE getstate() const {
				return _state;
			}

			void setstate(const STATE& state) {
				_state = state;
			}

			void reset() {
				setstate(_startstate);
			}

		private:
			statemachine(const STATE& startstate, const STATE& endstate, const std::map<STATE, state >& mapstate) 
				: _startstate(startstate), 
				  _endstate(endstate),
				  _state(startstate),
				  _mapstate(mapstate) {
			}

			statemachine& add(Int2Type<NOEXIST_STATE>, const STATE& st, const MSG& m, FUNC func) {
				typename MAP_MSGSTATUS::iterator it = _mapstate.find(st);
				if (it != _mapstate.end()) {
					it->second.Add(m, msg(func));
				}
				else {
					_mapstate.insert(
						MAP_VALUE(MAP_MSGSTATUS, st, 
										state().add(m, msg(func))));
				}
				return *this;
			}

			statemachine& add(Int2Type<NOEXIST_STATE>, const STATE& st, FUNC func) {
				typename MAP_MSGSTATUS::iterator it = _mapstate.find(st);
				if (it != _mapstate.end()) {
					it->second.Add(msg(func));
				}
				else {
					_mapstate.insert(
						MAP_VALUE(MAP_MSGSTATUS, st, 
										state().add(msg(func))));
				}
				return *this;
			}

			enumFLOWSTATE do_it(const MSG& m, Int2Type<INSIDE_PARAM>) {
				typename MAP_MSGSTATUS::iterator it = _mapstate.find(_state);
				if (it != _mapstate.end()) {
					it->second.do_it(m, _data._param, _state);
					return _state == _endstate ? END_STATE : NORMAL_STATE;
				}
				else {
					//assert(0);
					return ERROR_STATE;
				}
			}

			enumFLOWSTATE do_it(const MSG& m, PARAM& param, Int2Type<OUTSIDE_PARAM>) {
				typename MAP_MSGSTATUS::iterator it = _mapstate.find(_state);
				if (it != _mapstate.end()) {
					it->second.do_it(m, param, _state);
					return _state == _endstate ? END_STATE : NORMAL_STATE;
				}
				else {
					//assert(0);
					return ERROR_STATE;
				}
			}

		private:
			const STATE _endstate;
			const STATE _startstate;
			STATE _state;
			typedef std::map<STATE, state> MAP_MSGSTATUS;
			MAP_MSGSTATUS _mapstate;
#ifdef _MSC_VER
			data<enumParam> _data;
#else
			data<enumParam, PARAM> _data;
#endif
		}; //end class statemachine
	};
};

#endif

