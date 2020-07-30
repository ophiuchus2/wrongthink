#include <string>
#include "soci.h"
#include "soci-postgresql.h"

namespace WrongthinkUtils {
  /**
  * @brief sets the database credentials, should only be called once from the
  * main thread at initialization.
  * @param [in] user db username.
  * @param [in] pass db user password.
  */
  void setCredentials(const std::string& user, const std::string& pass);
  soci::session getSociSession();
  void validateDatabase();
  extern std::string dbUname_;
  extern std::string dbPass_;
}
