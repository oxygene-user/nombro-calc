#pragma once

#include <mutex>
#include <shared_mutex>

template <typename T> class aa // atomic access
{
	mutable std::shared_mutex m;
	T var;

	class reader
	{
		const T *var;
		const aa *host;
		friend class aa;
		reader(const T & _var, const aa *_host) : var(&_var), host(_host)
		{
		}
		reader & operator = (const reader &) = delete;
		reader(const reader &) = delete;
	public:
		reader(reader &&r) : var(r.var), host(r.host)
		{
			r.var = nullptr;
			r.host = nullptr;
		}
		~reader()
		{
			if (host)
				host->unlock_read();
		}
		reader & operator = (reader &&r)
		{
			if (host)
				host->unlock_read();
			var = r.var;
			host = r.host;
			r.var = nullptr;
			r.host = nullptr;
			return *this;
		}

		bool is_locked() const
		{
			return host != nullptr;
		}

		void unlock()
		{
			ASSERT(host != nullptr);
			host->unlock_read();
			var = nullptr;
			host = nullptr;
		}

		const T &operator()()
		{
			ASSERT(var != nullptr && host != nullptr);
			return *var;
		}
	};

	class writer
	{
		T * var;
		const aa *host;
		friend class aa;
		writer(T * _var, const aa *_host) : var(_var), host(_host)
		{
		}
		writer & operator = (const writer &r) = delete;
		writer(const writer &r) = delete;
	public:
		writer() : var(nullptr), host(nullptr) {}
		writer(writer &&r) : var(r.var), host(r.host)
		{
			r.var = nullptr;
			r.host = nullptr;
		}
		writer & operator = (writer &&r)
		{
			if (host != nullptr)
				host->unlock_write();
			var = r.var;
			host = r.host;
			r.var = nullptr;
			r.host = nullptr;
			return *this;
		}
		~writer()
		{
			if (host != nullptr)
				host->unlock_write();
		}
		bool is_locked() const { return host != nullptr; }
		operator bool() const { return is_locked(); }
		void unlock()
		{
			ASSERT(host != nullptr);
			host->unlock_write();
			var = nullptr;
			host = nullptr;
		}


		T &operator()()
		{
			ASSERT(var != nullptr && host != nullptr);
			return *var;
		}
	};


	friend class reader;
	friend class writer;

private:

	/**
	* Unlock variable (other threads can lock it)
	*/
	void unlock_write() const
	{
		m.unlock();
	}
	void unlock_read() const
	{
		m.unlock_shared();
	}

public:

	typedef reader READER;
	typedef writer WRITER;

public:


	/**
	* Constructor. Custom initialization for protected variable
	*/
	aa(const T &v) : var(v)
	{
	}
	/**
	* Constructor. Default initialization for protected variable
	*/
	aa()
	{
	}
	/**
	* Destructor
	*/
	~aa()
	{
	}

	/**
	* Sync variable for read. Other threads can also lock this variable for read.
	* Current thread will wait for lock_read if there are some other thread waits for lock_write
	*/

	reader lock_read() const
	{
		m.lock_shared();
		return reader(var, this);
	}

	/**
	*  Lock variable for write. no any other thread can lock for read or write.
	*/
	writer lock_write(bool trylock = false)
	{
		if (trylock)
		{
			if (m.try_lock())
			{
				return writer(&var, this);
			}
			return writer();
		}
		m.lock();
		return writer(&var, this);
	}
};


