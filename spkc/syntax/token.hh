#ifndef _SPKC_SYNTAX_TOKEN_HH
#define _SPKC_SYNTAX_TOKEN_HH

#include <cstdint>
#include <memory>
#include <string>
#include <typeinfo>

namespace SpkC {
	namespace Syntax {
		class IToken {
		public:
			virtual inline ~IToken() {}
		};

		template <typename T>
		class Token : public IToken {
		public:
			T data;

			inline Token() {}
			inline Token(T data) {
				this->data = data;
			}
			inline Token(T& data) {
				this->data = data;
			}
			inline std::string getTypeName() {
				return typeid(T).name();
			}
			virtual inline ~Token() {
				printf("Destructing token: %s\n", getTypeName().c_str());
			}
		};
	}
}

#define YYSTYPE std::shared_ptr<SpkC::Syntax::IToken>

#endif
