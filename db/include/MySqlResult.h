#ifndef MYSQLRESULT_H
#define MYSQLRESULT_H

#include "Global.h"

#include <boost/mysql/results.hpp>

namespace flaw {
    class FLAW_API MySqlResult {
	public:
		MySqlResult(boost::mysql::results& results);

		template <typename T>
		T Get(size_t row, size_t column) const {
			throw std::runtime_error("Unsupported type");
		}

		template <>
		std::string Get<std::string>(size_t row, size_t column) const {		
			auto ret = _results.rows().at(row).at(column);
			if (ret.is_string()) {
				return ret.as_string();
			}
			else if (ret.is_null()) {
				return "";
			}
			else {
				throw std::runtime_error("Unsupported type");
			}
		}

		template <>
		int64_t Get<int64_t>(size_t row, size_t column) const {
			auto ret = _results.rows().at(row).at(column);
			if (ret.is_int64()) {
				return ret.as_int64();
			}
			else if (ret.is_null()) {
				return 0;
			}
			else {
				throw std::runtime_error("Unsupported type");
			}
		}

		template <>
		double Get<double>(size_t row, size_t column) const {
			auto ret = _results.rows().at(row).at(column);
			if (ret.is_double()) {
				return ret.as_double();
			}
			else if (ret.is_null()) {
				return 0.0;
			}
			else {
				throw std::runtime_error("Unsupported type");
			}
		}

		inline boost::mysql::rows_view::const_iterator CBegin() const noexcept {
			return _results.rows().begin();
		}

		inline boost::mysql::rows_view::const_iterator CEnd() const noexcept {
			return _results.rows().end();
		}

		inline size_t GetRowCount() const {
			return _results.rows().size();
		}

		inline bool Empty() const {
			return _results.rows().empty();
		}

	private:
		boost::mysql::results& _results;
	};
}

#endif