#ifndef ODB_TRACER_HPP_
#define ODB_TRACER_HPP_

#include <odb/tracer.hxx>
#include <odb/connection.hxx>
#include <odb/statement.hxx>
#include <iostream>

class SQLTracer : public odb::tracer {
public:
    // ✅ Правильная сигнатура prepare
    virtual void prepare(odb::connection&, const odb::statement& stmt) override {
        std::cout << "[Prepare] " << stmt.text() << std::endl;
    }
    
    // ✅ Правильная сигнатура execute (с statement)
    virtual void execute(odb::connection&, const odb::statement& stmt) override {
        std::cout << "[Execute Statement] " << stmt.text() << std::endl;
    }
    
    // ✅ Правильная сигнатура execute (с const char*)
    virtual void execute(odb::connection&, const char* statement) override {
        std::cout << "[Execute SQL] " << statement << std::endl;
    }
    
    // ✅ Правильная сигнатура deallocate
    virtual void deallocate(odb::connection&, const odb::statement& stmt) override {
        std::cout << "[Deallocate] " << stmt.text() << std::endl;
    }
};

#endif
