#ifndef USER_TABLE_HXX_
#define USER_TABLE_HXX_

#include <string>
#include <odb/core.hxx>

namespace db {
    #pragma db object
    class User {
    public:
        User() {}

        User(const std::string& uuid, 
            const std::string& username, 
            const std::string& password) :
            uuid_(uuid), username_(username), password_(password) {}

        std::string uuid() const { return uuid_; }
        void uuid(const std::string& uuid) { uuid_ = uuid; }

        std::string username() const { return username_; }
        void username(const std::string& username) { username_ = username; }

        std::string password() const { return password_; }
        void password(const std::string& password) { password_ = password; }

    private:
        friend class odb::access;

        #pragma db id 
        std::string uuid_;
        
        #pragma db not_null
        #pragma db unique
        std::string username_;
        #pragma db not_null
        std::string password_;
    };   
};

#endif