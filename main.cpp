#include "engine/db.hpp"
#include <iostream>
#include <cassert>
#include <cstdio>
using namespace std;

int main(){
    using namespace TSDB;

    const string wal_file = "tsdb.wal";
    remove(wal_file.c_str());

    cout<<"PHASE-I INGESTION AND MEMTABLE "<<endl;
    {
    Database db("tsdb.wal");
    uint64_t ts1 = 1711002200;
    uint64_t ts2 = 1711002205;
    db.put("cpu.idle","server-01",ts1,98.4);
    db.put("cpu.idle","server-02",ts2,45.2);
    db.put("memory-use","server-01",ts2,72.1);

    double val = 0.0;

    if(db.get("cpu.idle","server-01",ts1,val)){
        cout<<"FOUND cpu.idle at server-01 "<<val<<endl;
        assert(val == 98.4);
    }else{
        cout<<"ERROR"<<endl;
    }

    if(db.get("cpu.idle","server-02",ts2,val)){
        cout<<"FOUND cpu.idle at server-01 "<<val<<endl;
        assert(val == 45.2);
    }else{
        cout<<"ERROR"<<endl;
    }

    // if (!db.get("cpu.idle", "non-existent-host", ts1, val)) {
    //         std::cout << "[SUCCESS] Correctly returned false for non-existent key.\n";
    // }
}

cout<<"PHASE-II RECOVERY "<<endl;
{
    Database db_recovered("tsdb.wal");

    uint64_t ts1 = 1711002200;
    uint64_t ts2 = 1711002205;
    double val = 0.0;

    if (db_recovered.get("cpu.idle", "server-01", ts1, val)) {
            std::cout << "[RECOVERY SUCCESS] Restored cpu.idle @ server-01: " << val << "\n";
            assert(val == 98.4);
        } else {
            std::cerr << "[RECOVERY FAILURE] cpu.idle @ server-01 missing after restart!\n";
        }

        if (db_recovered.get("memory.used", "server-01", ts2, val)) {
            std::cout << "[RECOVERY SUCCESS] Restored memory.used @ server-01: " << val << "\n";
            assert(val == 72.1);
        } else {
            std::cerr << "[RECOVERY FAILURE] memory.used @ server-01 missing after restart!\n";
        }
    
        cout<<db_recovered.size_bytes()<<endl;
}



return 0;

}