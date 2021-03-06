#include "ssyp_db.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "datamodel.h"

class Transaction : public ITransaction {
public:
    Operations ops;
};

class SsypDB : public ISsypDB {
public:
    SsypDB(DbSettings settings)
        : datamodel_(CreateDatamodel(CreateStorage(settings), settings)) {
        commit_thread_ = std::thread(&SsypDB::CommitThreadCycle, this);
    }
    ~SsypDB() {
        stop_thread_ = true;
        queue_check_.notify_all();
        commit_thread_.join();
    }
    ITransactionPtr StartTransaction() override {
        return std::make_shared<Transaction>();
    }

    void SetValue(std::string key, std::string value,
                  ITransactionPtr& tx) override {
        ((Transaction*)(tx.get()))
            ->ops.push_back({key, value, Op::Type::Update});
    }
    void SetValue(std::string key, const char* value,
                  ITransactionPtr& tx) override {
        ((Transaction*)(tx.get()))
            ->ops.push_back({key, value, Op::Type::Update});
    }
    void SetValue(std::string key, int value, ITransactionPtr& tx) override {
        ((Transaction*)(tx.get()))
            ->ops.push_back({key, std::to_string(value), Op::Type::Update});
    }
    void SetValue(std::string key, double value, ITransactionPtr& tx) override {
        ((Transaction*)(tx.get()))
            ->ops.push_back({key, std::to_string(value), Op::Type::Update});
    }
    void SetValue(std::string key, bool value, ITransactionPtr& tx) override {
        ((Transaction*)(tx.get()))
            ->ops.push_back({key, value ? "true" : "false", Op::Type::Update});
    }

    void Remove(std::string key, ITransactionPtr& tx) override {
        ((Transaction*)(tx.get()))->ops.push_back({key, "", Op::Type::Remove});
    }

    bool GetValue(std::string key, std::string& value) override {
        return datamodel_->GetValue(key, value);
    }
    bool GetValue(std::string key, int& value) override {
        std::string tmp;
        datamodel_->GetValue(key, tmp);
        value = std::stoi(tmp);
        return true;
    }
    bool GetValue(std::string key, double& value) override {
        std::string tmp;
        datamodel_->GetValue(key, tmp);
        value = std::stod(tmp);
        return true;
    }
    bool GetValue(std::string key, bool& value) override {
        std::string tmp;
        if (!datamodel_->GetValue(key, tmp)) return false;
        if (tmp == "true") {
            value = true;
            return true;
        } else if (tmp == "false") {
            value = false;
            return true;
        } else {
            return false;
        }
    }
    std::future<CommitStatus> Commit(ITransactionPtr tx) override {
        std::promise<CommitStatus> promise;
        std::future<CommitStatus> future = promise.get_future();
        std::lock_guard lock(mutex_);
        transaction_ptr_queue_.push_back(
            std::make_pair(tx, std::move(promise)));
        queue_check_.notify_one();
        return future;
    }

private:
    IDatamodelPtr datamodel_;
    std::vector<std::pair<ITransactionPtr, std::promise<CommitStatus>>>
        transaction_ptr_queue_;
    std::atomic_bool stop_thread_ = false;
    std::thread commit_thread_;
    std::mutex mutex_;
    std::condition_variable queue_check_;

    void CommitThreadCycle() {
        while (!stop_thread_.load()) {
            std::unique_lock<std::mutex> locker(mutex_);
            queue_check_.wait(locker, [&]() {
                return !transaction_ptr_queue_.empty() || stop_thread_;
            });
            decltype(transaction_ptr_queue_) local_transaction_queue;
            std::swap(local_transaction_queue, transaction_ptr_queue_);
            locker.unlock();
            for (auto& [tx, promise] : local_transaction_queue) {
                auto status =
                    datamodel_->Commit(((Transaction*)(tx.get()))->ops)
                        ? CommitStatus::Success
                        : CommitStatus::Error;
                promise.set_value(status);
            }
        }
        std::lock_guard lock(mutex_);
        if (!transaction_ptr_queue_.empty()) {
            for (auto& [tx, promise] : transaction_ptr_queue_) {
                promise.set_value(CommitStatus::Error);
            }
        }
    }
};

std::shared_ptr<ISsypDB> CreateDb(DbSettings settings) {
    return std::make_shared<SsypDB>(settings);
}
