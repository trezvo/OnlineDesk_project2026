#ifndef TABLES_MODELS_HXX_
#define TABLES_MODELS_HXX_

#include <odb/core.hxx>
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace db {
    #pragma db object
    class User {
    public:
        User() {}

        User(const std::string& uuid, 
            const std::string& username, 
            const std::string& password) :
            uuid_(uuid), username_(username), password_(password) {}

        std::string& uuid() { return uuid_; }
        void uuid(const std::string& uuid) { uuid_ = uuid; }

        std::string& username() { return username_; }
        void username(const std::string& username) { username_ = username; }

        std::string& password() { return password_; }
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

    #pragma db object
    class Board {
        public:
        Board() {}

        Board(const std::string& name) : name_(name) {}

        unsigned long long& id() { return id_; }
        void id(unsigned long long id) { id_ = id; }

        std::string& name() { return name_; }
        void name(const std::string& name) { name_ = name; }

        std::shared_ptr<User>& owner() { return owner_; }
        void owner(std::shared_ptr<User> owner) { owner_ = owner; }

        private:
            friend class odb::access;

            #pragma db id auto
            unsigned long long id_;

            #pragma db not_null
            #pragma db unique
            std::string name_;

            #pragma db not_null on_delete(cascade) 
            std::shared_ptr<User> owner_;
    };

    #pragma db object
    class Widget {

        public:
        Widget() {}
        Widget(unsigned long long id, 
            int x, 
            int y, 
            const std::string& content,
            std::shared_ptr<Board> board) : 
        id_(id), x_coord_(x), y_coord_(y), content_(content), board_(board) {}

        explicit Widget(unsigned long long id) : id_(id) {}
        explicit Widget(const std::string& content) : content_(content) {}

        const unsigned long long& id() const { return id_; }
        unsigned long long& id() { return id_; }
        void id(unsigned long long id) { id_ = id; }

        int x() const { return x_coord_; }
        int x() {return x_coord_; }
        void x(int x) {x_coord_ = x; }

        int y() const { return y_coord_; }
        int y() {return y_coord_; }
        void y(int y) { x_coord_ = y; }

        const std::string& content() const { return content_; }
        std::string& content() { return content_; }
        void content(const std::string& content) { content_ = content; }

        const std::shared_ptr<Board>& board() const { return board_; }
        std::shared_ptr<Board>& board() { return board_; }
        void board(std::shared_ptr<Board> board) { board_ = board; }

        private:
            friend class odb::access;

            #pragma db id
            unsigned long long id_;

            #pragma db not_null
            int x_coord_;
            #pragma db not_null
            int y_coord_;

            std::string content_;

            #pragma db not_null on_delete(cascade)
            std::shared_ptr<Board> board_;
    };
}

#endif
