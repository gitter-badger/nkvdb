#pragma once

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include "page.h"
#include "cache.h"
#include "asyncworker.h"

namespace nkvdb {

const uint64_t defaultPageSize = sizeof(Page::Header) + sizeof(Meas) * 1000000;
const size_t defaultcacheSize = 10000;
const size_t defaultcachePoolSize = 100;

class StorageReader;
typedef std::shared_ptr<StorageReader> StorageReader_ptr;

class Storage;

class AsyncWriter : public utils::AsyncWorker<Cache::PCache> {
public:
  AsyncWriter();
  void setStorage(Storage *nkvdb);
  void call(const Cache::PCache data) override;

private:
  Storage *m_storage;
};

/**
* Main class of nkvdb storage.
*/
class Storage {
public:
    typedef std::shared_ptr<Storage> Storage_ptr;

public:
    static Storage_ptr Create(const std::string &ds_path, uint64_t page_size = defaultPageSize);
    static Storage_ptr Open(const std::string &ds_path);
    ~Storage();
    void Close();

    bool havePage2Write() const;
    append_result append(const Meas& m);
    append_result append(const Meas::PMeas begin, const size_t meas_count);

    StorageReader_ptr readInterval(Time from, Time to);
    StorageReader_ptr readInterval(const IdArray &ids, nkvdb::Flag source, nkvdb::Flag flag, Time from, Time to);
    Meas::MeasList curValues(const IdArray&ids);

    StorageReader_ptr readInTimePoint(Time time_point);
    StorageReader_ptr readInTimePoint(const IdArray &ids, nkvdb::Flag source, nkvdb::Flag flag, Time time_point);

    /// get max time in past to write
    Time pastTime() const;
    /// set max time in past to write
    void setPastTime(const Time &t);

    void enableCacheDynamicSize(bool flg);
    bool cacheDynamicSize() const;

    size_t getPoolSize()const;
    void setPoolSize(size_t sz);

    size_t getCacheSize()const;
    void setCacheSize(size_t sz);

    /// load current values of ids. return array of not founded measurements.
    IdArray loadCurValues(const IdArray&ids);
private:
    Storage();
    void writeCache();
    void flush_and_stop();
protected:
    std::string m_path;

    std::mutex m_write_mutex;
    nkvdb::Cache::PCache m_cache;
    AsyncWriter m_cache_writer;
    CachePool m_cache_pool;
    CurValuesCache m_cur_values;
    Time m_past_time;
    bool m_closed;
    friend class nkvdb::Cache;
};

class StorageReader: public utils::NonCopy{
public:
    StorageReader();
    bool isEnd();
    void readNext(Meas::MeasList*output);
    void readAll(Meas::MeasList*output);
    void addPage(std::string page_name);

    IdArray ids;
    nkvdb::Flag source;
    nkvdb::Flag flag;
    nkvdb::Time from;
    nkvdb::Time to;
    nkvdb::Time time_point;
    std::string prev_interval_page;
private:
    std::deque<std::string> m_pages;
    PageReader_ptr m_current_reader;
};
}
