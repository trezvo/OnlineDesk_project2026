#ifndef TABLES_MODELS_HXX_
#define TABLES_MODELS_HXX_

#include <string>
#include <memory>
#include <vector>
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

        std::vector<std::weak_ptr<Board>> owned_boards() const { return owned_boards_; }
        void owned_boards(const std::vector<std::weak_ptr<Board>>& owned_boards) 
            { owned_boards_ = owned_boards; }

    private:
        friend class odb::access;

        #pragma db id 
        std::string uuid_;
        
        #pragma db not_null
        #pragma db unique
        std::string username_;
        #pragma db not_null
        std::string password_;

        #pragma db inverse(owner_)
        std::vector<std::weak_ptr<Board>> owned_boards_;
    };   

    #pragma db object
    class Board {
        public:
        Board() : id_(0) {}

        Board(const std::string& name) : name_(name) {}

        unsigned long id() const { return id_; }
        void id(unsigned long id) { id_ = id; }

        std::string name() const { return name_; }
        void name(const std::string& name) { name_ = name; }

        std::shared_ptr<User> owner() const { return owner_; }
        void owner(std::shared_ptr<User> owner) { owner_ = owner; }

        private:
            friend class odb::access;

            #pragma db id auto
            unsigned long id_;

            #pragma db not_null
            #pragma db unique
            std::string name_;

            #pragma db not_null on_delete(cascade) 
            std::shared_ptr<User> owner_;
    };
}

#endif
