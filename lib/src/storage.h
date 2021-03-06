#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../include/settings.h"

class ITableList {
public:
    virtual size_t TableCount() const = 0;

    // Возвращает таблицу в виде бинарного блоба.
    // TODO: возвращать интерфейс для чтения из файла
    virtual std::string GetTable(size_t index) const = 0;
};
using ITableListPtr = std::shared_ptr<ITableList>;

using JournalBlob = std::vector<std::string>;

class IStorage {
public:
    // Записывает операции в журнал. Каждый элемент вектора - отдельная
    // операция.
    virtual bool WriteToJournal(std::vector<std::string> ops) = 0;

    // Записывает новую таблицу в базу
    virtual bool PushJournalToTable(std::string blob) = 0;

    // Возвращает view интерфейс на текущие таблицы
    virtual ITableListPtr GetTableList() = 0;

    // Возвращает журнал в виде бинарных данных (может быть полезно при
    // восстановлении базы)
    virtual JournalBlob GetJournal() = 0;

    virtual bool MergeTable(std::vector<size_t> merged_tables,
                            std::string result_table) = 0;
};

struct StorageStatistic {
public:
    ~StorageStatistic();
    std::atomic_int write_journal_count_ = 0;
    std::atomic_int read_journal_count_ = 0;
    std::atomic_int push_table_count_ = 0;
    std::atomic_int merge_table_count_ = 0;
    std::atomic_int read_table_count_ = 0;
};

using IStoragePtr = std::shared_ptr<IStorage>;

IStoragePtr CreateStorage(DbSettings settings);
