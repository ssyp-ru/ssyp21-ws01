#pragma once
#include <mutex>

#include "storage.h"

class InMemoryTableList : public ITableList {
public:
    InMemoryTableList(std::vector<std::shared_ptr<std::string>> tables,
                      std::atomic_int& read_table_count,
                      std::shared_ptr<std::mutex> mtx);
    size_t TableCount() const override;
    std::string GetTable(size_t index) const override;

private:
    std::vector<std::shared_ptr<std::string>> tables_;
    std::atomic_int& read_table_count_;
    std::shared_ptr<std::mutex> mutex_;
};

class InMemoryStorage : public IStorage {
public:
    bool WriteToJournal(std::vector<std::string> ops) override;
    bool PushJournalToTable(std::string blob) override;
    ITableListPtr GetTableList() override;
    JournalBlob GetJournal() override;
    bool MergeTable(std::vector<size_t> merged_tables,
                    std::string result_table) override;

private:
    std::vector<std::string> journal_;
    std::vector<std::shared_ptr<std::string>> table_list_;
    StorageStatistic statistic_;
    std::shared_ptr<std::mutex> mutex_ = std::make_shared<std::mutex>();
};
