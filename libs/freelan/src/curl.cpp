/*
 * libfreelan - A C++ library to establish peer-to-peer virtual private
 * networks.
 * Copyright (C) 2010-2011 Julien KAUFFMANN <julien.kauffmann@freelan.org>
 *
 * This file is part of libfreelan.
 *
 * libfreelan is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * libfreelan is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 *
 * If you intend to use libfreelan in a commercial software, please
 * contact me : we may arrange this for a small fee or no fee at all,
 * depending on the nature of your project.
 */

/**
 * \file curl.cpp
 * \author Julien KAUFFMANN <julien.kauffmann@freelan.org>
 * \brief The curl structures and functions.
 */

#include "curl.hpp"

#include <cassert>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace freelan
{
	namespace
	{
		void throw_if_curl_error(CURLcode errorcode)
		{
			if (errorcode != CURLE_OK)
			{
				throw std::runtime_error(curl_easy_strerror(errorcode));
			}
		}

		void throw_if_curlm_error(CURLMcode errorcode)
		{
			if (errorcode != CURLM_OK)
			{
				throw std::runtime_error(curl_multi_strerror(errorcode));
			}
		}
	}

	curl_list::curl_list() :
		m_slist(nullptr, [](curl_slist* p){ if (p) { curl_slist_free_all(p); }})
	{
	}

	void curl_list::append(const std::string& value)
	{
		struct curl_slist* const new_slist = curl_slist_append(m_slist.get(), value.c_str());

		if (!new_slist)
		{
			throw std::runtime_error("Unable to append a value to the list");
		}

		m_slist.reset(new_slist);
	}

	void curl_list::reset()
	{
		m_slist.reset();
	}

	struct curl_slist* curl_list::raw() const
	{
		return m_slist.get();
	}

	curl::curl() :
		m_curl(curl_easy_init(), [] (CURL* p) { if (p) curl_easy_cleanup(p); }),
		m_debug_function()
	{
		if (!m_curl)
		{
			throw std::runtime_error("Unable to allocate a CURL structure");
		}
	}

	void curl::set_option(CURLoption option, void* value)
	{
		throw_if_curl_error(curl_easy_setopt(m_curl.get(), option, value));
	}

	void curl::set_option(CURLoption option, long int value)
	{
		throw_if_curl_error(curl_easy_setopt(m_curl.get(), option, value));
	}

	void curl::set_option(CURLoption option, curl_debug_callback value)
	{
		throw_if_curl_error(curl_easy_setopt(m_curl.get(), option, value));
	}

	void curl::set_option(CURLoption option, curl_write_callback value)
	{
		throw_if_curl_error(curl_easy_setopt(m_curl.get(), option, value));
	}

	void curl::set_option(CURLoption option, curl_open_socket_callback value)
	{
		throw_if_curl_error(curl_easy_setopt(m_curl.get(), option, value));
	}

	void curl::set_option(CURLoption option, curl_close_socket_callback value)
	{
		throw_if_curl_error(curl_easy_setopt(m_curl.get(), option, value));
	}

	void curl::set_proxy(const asiotap::endpoint& proxy)
	{
		if (proxy != asiotap::hostname_endpoint::null())
		{
			set_option(CURLOPT_PROXY, static_cast<const void*>(boost::lexical_cast<std::string>(proxy).c_str()));
		}
		else
		{
			set_option(CURLOPT_PROXY, static_cast<const void*>(NULL));
		}
	}

	void curl::set_debug_function(debug_function_t func)
	{
		m_debug_function = func;

		if (m_debug_function)
		{
			set_option(CURLOPT_DEBUGFUNCTION, &curl::debug_function);
			set_option(CURLOPT_DEBUGDATA, &m_debug_function);
		}
		else
		{
			set_option(CURLOPT_DEBUGFUNCTION, static_cast<void*>(NULL));
			set_option(CURLOPT_DEBUGDATA, static_cast<void*>(NULL));
		}
	}

	void curl::set_write_function(write_function_t func)
	{
		m_write_function = func;

		if (m_write_function)
		{
			set_option(CURLOPT_WRITEFUNCTION, &curl::write_function);
			set_option(CURLOPT_WRITEDATA, &m_write_function);
		}
		else
		{
			set_option(CURLOPT_WRITEFUNCTION, static_cast<void*>(NULL));
			set_option(CURLOPT_WRITEDATA, static_cast<void*>(NULL));
		}
	}

	void curl::set_user_agent(const std::string& user_agent)
	{
		set_option(CURLOPT_USERAGENT, static_cast<const void*>(user_agent.c_str()));
	}

	void curl::set_url(const std::string& url)
	{
		set_option(CURLOPT_URL, static_cast<const void*>(url.c_str()));
	}

	void curl::set_ssl_peer_verification(bool state)
	{
		set_option(CURLOPT_SSL_VERIFYPEER, state ? 1L : 0L);
	}

	void curl::set_ssl_host_verification(bool state)
	{
		set_option(CURLOPT_SSL_VERIFYHOST, state ? 2L : 0L);
	}

	void curl::set_ca_info(const boost::filesystem::path& ca_info)
	{
		if (ca_info.empty())
		{
			set_option(CURLOPT_CAINFO, static_cast<const void*>(NULL));
		}
		else
		{
			set_option(CURLOPT_CAINFO, static_cast<const void*>(ca_info.string().c_str()));
		}
	}

	void curl::set_connect_timeout(const boost::posix_time::time_duration& timeout)
	{
		set_option(CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(timeout.total_milliseconds()));
	}

	void curl::set_http_header(const std::string& header, const std::string& value)
	{
		m_http_headers.append(header + ": " + value);

		set_option(CURLOPT_HTTPHEADER, static_cast<const void*>(m_http_headers.raw()));
	}

	void curl::unset_http_header(const std::string& header)
	{
		m_http_headers.append(header + ":");

		set_option(CURLOPT_HTTPHEADER, static_cast<const void*>(m_http_headers.raw()));
	}

	void curl::reset_http_headers()
	{
		m_http_headers = curl_list();

		set_option(CURLOPT_HTTPHEADER, static_cast<const void*>(m_http_headers.raw()));
	}

	void curl::set_get()
	{
		set_option(CURLOPT_HTTPGET, 1L);
	}

	void curl::set_post()
	{
		set_option(CURLOPT_POST, 1L);
	}

	void curl::set_post_fields(boost::asio::const_buffer buf)
	{
		set_option(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<long>(boost::asio::buffer_size(buf)));
		set_option(CURLOPT_POSTFIELDS, boost::asio::buffer_cast<const void*>(buf));
	}

	void curl::set_copy_post_fields(boost::asio::const_buffer buf)
	{
		set_option(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<long>(boost::asio::buffer_size(buf)));
		set_option(CURLOPT_COPYPOSTFIELDS, boost::asio::buffer_cast<const void*>(buf));
	}

	void curl::set_cookie_file(const std::string& file)
	{
		set_option(CURLOPT_COOKIEFILE, file.c_str());
	}

	void curl::enable_cookie_support()
	{
		set_cookie_file("");
	}

	void curl::set_username(const std::string& username)
	{
		set_option(CURLOPT_USERNAME, username.c_str());
	}

	void curl::set_password(const std::string& password)
	{
		set_option(CURLOPT_PASSWORD, password.c_str());
	}

	std::string curl::escape(const std::string& url)
	{
		char* rstr = curl_easy_escape(m_curl.get(), url.c_str(), static_cast<int>(url.size()));

		if (!rstr)
		{
			throw std::bad_alloc();
		}

		boost::shared_ptr<char> str(rstr, curl_free);

		return std::string(str.get());
	}

	std::string curl::unescape(const std::string& encoded)
	{
		int len = 0;
		char* rstr = curl_easy_unescape(m_curl.get(), encoded.c_str(), static_cast<int>(encoded.size()), &len);

		if (!rstr)
		{
			throw std::bad_alloc();
		}

		boost::shared_ptr<char> str(rstr, curl_free);

		return std::string(str.get(), static_cast<size_t>(len));
	}

	void curl::perform()
	{
		throw_if_curl_error(curl_easy_perform(m_curl.get()));
	}

	long curl::get_response_code()
	{
		long response_code = 0;

		throw_if_curl_error(curl_easy_getinfo(m_curl.get(), CURLINFO_RESPONSE_CODE, &response_code));

		return response_code;
	}

	ptrdiff_t curl::get_content_length_download()
	{
		double content_length = 0.0;

		throw_if_curl_error(curl_easy_getinfo(m_curl.get(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length));

		return (content_length >= 0) ? static_cast<ptrdiff_t>(content_length) : -1;
	}

	ptrdiff_t curl::get_content_length_upload()
	{
		double content_length = 0.0;

		throw_if_curl_error(curl_easy_getinfo(m_curl.get(), CURLINFO_CONTENT_LENGTH_UPLOAD, &content_length));

		return (content_length >= 0) ? static_cast<ptrdiff_t>(content_length) : -1;
	}

	std::string curl::get_content_type()
	{
		char* content_type = NULL;

		throw_if_curl_error(curl_easy_getinfo(m_curl.get(), CURLINFO_CONTENT_TYPE, &content_type));

		return content_type ? content_type : "";
	}

	int curl::debug_function(CURL*, curl_infotype infotype, char* data, size_t datalen, void* context)
	{
		assert(context);

		debug_function_t& func = *static_cast<debug_function_t*>(context);

		func(infotype, boost::asio::buffer(data, datalen));

		return 0;
	}

	size_t curl::write_function(char* data, size_t size, size_t nmemb, void* context)
	{
		assert(context);

		write_function_t& func = *static_cast<write_function_t*>(context);

		return func(boost::asio::buffer(data, size * nmemb));
	}

	curl_multi::curl_multi() :
		m_curlm(curl_multi_init(), [](CURLM* p){ if (p) curl_multi_cleanup(p); })
	{
		if (!m_curlm)
		{
			throw std::runtime_error("Unable to allocate a CURLM structure");
		}
	}

	void curl_multi::add_handle(boost::shared_ptr<curl> handle)
	{
		m_associations[handle->raw()] = std::unique_ptr<curl_association>(new curl_association(*this, handle));
	}

	boost::shared_ptr<curl> curl_multi::remove_handle(CURL* easy_handle)
	{
		boost::shared_ptr<curl> result;
		const auto it = m_associations.find(easy_handle);

		if (it != m_associations.end())
		{
			result = it->second->get_curl();

			m_associations.erase(it);
		}

		return result;
	}

	void curl_multi::clear()
	{
		m_associations.clear();
	}

	void curl_multi::set_option(CURLMoption option, void* value)
	{
		throw_if_curlm_error(curl_multi_setopt(m_curlm.get(), option, value));
	}

	void curl_multi::set_option(CURLMoption option, curl_multi_timer_callback value)
	{
		throw_if_curlm_error(curl_multi_setopt(m_curlm.get(), option, value));
	}

	void curl_multi::set_option(CURLMoption option, curl_socket_callback value)
	{
		throw_if_curlm_error(curl_multi_setopt(m_curlm.get(), option, value));
	}

	void curl_multi::socket_action(curl_socket_t sockfd, int ev_bitmask, int* running_handles)
	{
		throw_if_curlm_error(curl_multi_socket_action(m_curlm.get(), sockfd, ev_bitmask, running_handles));
	}

	CURLMsg* curl_multi::info_read(int* count_left)
	{
		int local_counter = 0;

		if (!count_left)
		{
			count_left = &local_counter;
		}

		return curl_multi_info_read(m_curlm.get(), count_left);
	}

	curl_multi::curl_association::curl_association(const curl_multi& _curl_multi, boost::shared_ptr<curl> _curl) :
		m_curl_multi(_curl_multi),
		m_curl(_curl)
	{
		throw_if_curlm_error(curl_multi_add_handle(m_curl_multi.raw(), m_curl->raw()));
	}

	curl_multi::curl_association::~curl_association()
	{
		throw_if_curlm_error(curl_multi_remove_handle(m_curl_multi.raw(), m_curl->raw()));
	}

	curl_multi_asio::~curl_multi_asio()
	{
		set_option(CURLMOPT_SOCKETDATA, static_cast<void*>(nullptr));
		set_option(CURLMOPT_SOCKETFUNCTION, static_cast<void*>(nullptr));
		set_option(CURLMOPT_TIMERDATA, static_cast<void*>(nullptr));
		set_option(CURLMOPT_TIMERFUNCTION, static_cast<void*>(nullptr));
	}

	void curl_multi_asio::add_handle(boost::shared_ptr<curl> handle, connection_complete_callback handler)
	{
		handle->set_option(CURLOPT_OPENSOCKETFUNCTION, &curl_multi_asio::open_socket_callback);
		handle->set_option(CURLOPT_OPENSOCKETDATA, this);
		handle->set_option(CURLOPT_CLOSESOCKETFUNCTION, &curl_multi_asio::close_socket_callback);
		handle->set_option(CURLOPT_CLOSESOCKETDATA, this);

		curl_multi::add_handle(handle);

		m_handler_map[handle] = handler;
	}

	void curl_multi_asio::post_handle(boost::shared_ptr<curl> handle, connection_complete_callback handler)
	{
		m_strand.post([this, handle, handler] () {
			add_handle(handle, handler);
		});
	}

	boost::shared_ptr<curl> curl_multi_asio::remove_handle(CURL* easy_handle)
	{
		boost::shared_ptr<curl> result = curl_multi::remove_handle(easy_handle);

		m_handler_map.erase(result);

		result->set_option(CURLOPT_CLOSESOCKETDATA, static_cast<void*>(nullptr));
		result->set_option(CURLOPT_CLOSESOCKETFUNCTION, static_cast<void*>(nullptr));
		result->set_option(CURLOPT_OPENSOCKETDATA, static_cast<void*>(nullptr));
		result->set_option(CURLOPT_OPENSOCKETFUNCTION, static_cast<void*>(nullptr));

		return result;
	}

	void curl_multi_asio::clear()
	{
		curl_multi::clear();

		m_handler_map.clear();
		m_socket_map.clear();
	}

	void curl_multi_asio::async_clear(boost::function<void ()> handler)
	{
		m_strand.post([this, handler] () {
			clear();

			if (handler)
			{
				handler();
			}
		});
	}

	int curl_multi_asio::static_timer_callback(CURLM*, long timeout_ms, void* _curl_multi_asio)
	{
		assert(_curl_multi_asio);

		curl_multi_asio& self = *static_cast<curl_multi_asio*>(_curl_multi_asio);

		self.m_timer.cancel();

		if (timeout_ms > 0)
		{
			self.m_timer.expires_from_now(boost::posix_time::millisec(timeout_ms));
			self.m_timer.async_wait(self.m_strand.wrap(boost::bind(&curl_multi_asio::timer_callback, self.shared_from_this(), _1)));
		}
		else
		{
			self.m_strand.post(boost::bind(&curl_multi_asio::timer_callback, self.shared_from_this(), boost::system::error_code()));
		}

		return 0;
	}

	int curl_multi_asio::static_socket_callback(CURL*, curl_socket_t socket_fd, int action, void* _curl_multi_asio, void*)
	{
		assert(_curl_multi_asio);

		curl_multi_asio& self = *static_cast<curl_multi_asio*>(_curl_multi_asio);

		const auto socket = self.m_socket_map[socket_fd];

		if (socket)
		{
			switch (action)
			{
				case CURL_POLL_REMOVE:
				{
					std::cout << "CURL remove: " << socket_fd << std::endl;
					break;
				}
				case CURL_POLL_IN:
				{
					std::cout << "CURL in: " << socket_fd << std::endl;
					break;
				}
				case CURL_POLL_OUT:
				{
					std::cout << "CURL out: " << socket_fd << std::endl;
					break;
				}
				case CURL_POLL_INOUT:
				{
					std::cout << "CURL in/out: " << socket_fd << std::endl;
					break;
				}
			}
		}

		return 0;
	}

	curl_socket_t curl_multi_asio::open_socket_callback(void* _curl_multi_asio, curlsocktype purpose, struct curl_sockaddr* address)
	{
		assert(_curl_multi_asio);

		curl_multi_asio& self = *static_cast<curl_multi_asio*>(_curl_multi_asio);

		if ((purpose == CURLSOCKTYPE_IPCXN) && ((address->family == AF_INET) || (address->family == AF_INET6)))
		{
			boost::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket = boost::make_shared<boost::asio::ip::tcp::socket>(
				self.m_io_service,
				(address->family == AF_INET) ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6()
			);

			const curl_socket_t socket_fd = tcp_socket->native_handle();
			self.m_socket_map[socket_fd] = tcp_socket;

			return socket_fd;
		}

		return CURL_SOCKET_BAD;
	}

	int curl_multi_asio::close_socket_callback(void* _curl_multi_asio, curl_socket_t socket_fd)
	{
		assert(_curl_multi_asio);

		curl_multi_asio& self = *static_cast<curl_multi_asio*>(_curl_multi_asio);

		self.m_socket_map.erase(socket_fd);

		return 0;
	}

	curl_multi_asio::curl_multi_asio(boost::asio::io_service& io_service) :
		m_io_service(io_service),
		m_strand(m_io_service),
		m_timer(m_io_service)
	{
		set_option(CURLMOPT_TIMERFUNCTION, &curl_multi_asio::static_timer_callback);
		set_option(CURLMOPT_TIMERDATA, this);
		set_option(CURLMOPT_SOCKETFUNCTION, &curl_multi_asio::static_socket_callback);
		set_option(CURLMOPT_SOCKETDATA, this);
	}

	void curl_multi_asio::timer_callback(const boost::system::error_code& ec)
	{
		if (!ec)
		{
			int running_handles = 0;

			socket_action(CURL_SOCKET_TIMEOUT, 0, &running_handles);
			check_info();
		}
	}

	void curl_multi_asio::check_info()
	{
		for (CURLMsg* msg = info_read(); msg; msg = info_read())
		{
			if (msg->msg == CURLMSG_DONE)
			{
				boost::shared_ptr<curl> _curl = remove_handle(msg->easy_handle);
				const auto handler_it = m_handler_map.find(_curl);

				if (handler_it != m_handler_map.end())
				{
					const auto handler = handler_it->second;

					if (handler)
					{
						m_io_service.post(boost::bind(handler, boost::system::error_code()));
					}

					m_handler_map.erase(handler_it);
				}
			}
		}
	}

	curl_manager::curl_manager(boost::asio::io_service& io_service) :
		m_curl_multi_asio(curl_multi_asio::create(io_service))
	{
	}

	curl_manager::~curl_manager()
	{
		m_curl_multi_asio->async_clear();
	}

	void curl_manager::execute(boost::shared_ptr<curl> _curl, connection_complete_callback handler)
	{
		m_curl_multi_asio->post_handle(_curl, handler);
	}
}
