#ifdef _WIN32

#ifdef CLUSTERFQCL_API_EXPORTS
#define CLUSTERFQCL_API __declspec(dllexport) 
#else
#define CLUSTERFQCL_API __declspec(dllimport) 
#endif

#else 
#define CLUSTERFQCL_API

#define EXPORT __attribute__((visibility("default")))
#define IMPORT

#endif

#include <string>
#include <vector>

using namespace std;

namespace ClusterFQ {
	class ClusterFQ	{
		private:
			static vector<string>	errors_v;

		public:
			static CLUSTERFQCL_API bool								init(string address, int port);

			static CLUSTERFQCL_API string							query(const char *module, const char *module_action, int paramset_id);

			static CLUSTERFQCL_API int								paramset_create();
			static CLUSTERFQCL_API bool								paramset_param_add(int id, const char* param, int value);
			static CLUSTERFQCL_API bool								paramset_param_add(int id, const char* param, long long value);
			static CLUSTERFQCL_API bool								paramset_param_add(int id, const char* param, string value);

			static CLUSTERFQCL_API void *							shared_memory_buffer_connect_i(string name, int size, int slots, unsigned int drop_skip);
			static CLUSTERFQCL_API int								shared_memory_buffer_write_slot_i(void *smb, int slot, unsigned char* data, unsigned int data_len);
			static CLUSTERFQCL_API bool								shared_memory_buffer_read_slot_i(void *smb, int slot, unsigned char* data);

			static CLUSTERFQCL_API string							errors();
	};
}
