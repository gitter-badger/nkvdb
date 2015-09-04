#include "cache.h"
#include <iostream>

using namespace storage;

Cache::Cache(size_t size) :m_max_size(size), m_size(0), m_index(0), m_sync(false) {
    m_meases=new Meas[size];
}


Cache::~Cache() {
    delete[] m_meases;
}

bool Cache::isFull()const {
    return m_size == m_max_size;
}

void Cache::clear() {
    //	m_data.clear();
    m_size=0;
    m_index = 0;
}

bool Cache::is_sync() const{
    return m_sync;
}

void Cache::sync_begin() {
    m_sync=true;
}

void Cache::sync_complete(){
    m_sync=false;
}

append_result Cache::append(const Meas& value, const Time past_time) {
	append_result res{};
	
	if (!checkPastTime(value.time, past_time)) {
		res.writed = 1;
		res.ignored = 1;
		return res;
	}
    //std::lock_guard<std::mutex> lock(this->m_rw_lock);
    if (!isFull()) {
        m_meases[m_index] = value;
        //this->m_data[value.time].push_back(m_index);
        m_index++;
        m_size++;

		res.writed = 1;
		res.ignored = 0;
        return res;
    } else {
        return res;
    }
}

append_result Cache::append(const Meas::PMeas begin, const size_t size, const Time past_time) {
    //std::lock_guard<std::mutex> lock(this->m_rw_lock);
    size_t cap = this->m_max_size;
    size_t to_write = 0;
    if (cap > size) {
        to_write = size;
    } else if (cap == size) {
        to_write = size;
    } else if (cap < size) {
        to_write = cap;
    }

	append_result res{};
	res.writed = to_write;
	
    for (size_t i = 0; i < to_write; ++i) {
		if (!checkPastTime(begin[i].time, past_time)) {
			res.ignored++;
			continue;
		}
        m_meases[m_index] = Meas{ begin[i] };
        m_size++;
        m_index++;
    }

	return res;
}

Meas::MeasList Cache::readInterval(Time from, Time to) const {
    //std::lock_guard<std::mutex> lock(this->m_rw_lock);
    Meas::MeasList result;
    for (size_t i = 0; i<m_index; ++i) {
        if (utils::inInterval(from, to, m_meases[i].time)) {
            result.push_back(m_meases[i]);
        }
    }
    return result;
}

Meas::PMeas Cache::asArray()const {
    //std::lock_guard<std::mutex> lock(this->m_rw_lock);
    //int i = 0;
    /*for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        for (auto m : it->second) {
            result[i]=m_meases[m];
            ++i;
        }
    }*/

    //    for (auto i = 0; i<m_index; ++i) {
    //        output[i] = m_meases[i];
    //    }
    return m_meases;
}


CachePool::CachePool(const size_t pool_size, const size_t cache_size):
    m_pool_size(pool_size),
    m_cache_size(cache_size)
{
    this->resize(m_pool_size);
    for(size_t i=0;i<m_pool_size;++i){
        Cache::PCache c(new Cache(m_cache_size));
        (*this)[i]=c;
    }
}


bool CachePool::haveCache()const{
    auto c=this->getCache();
    return c!=nullptr;
}

Cache::PCache CachePool::getCache()const{
    for(size_t i=0;i<this->size();i++){
        if (!this->at(i)->is_sync()){
            return this->at(i);
        }
    }

    return nullptr;
}
