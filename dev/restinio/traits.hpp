/*
	restinio
*/

/*!
	HTTP server traits.
*/

#pragma once

#include <restinio/request_handler.hpp>
#include <restinio/asio_timer_manager.hpp>
#include <restinio/null_logger.hpp>
#include <restinio/connection_state_listener.hpp>
#include <restinio/ip_blocker.hpp>
#include <restinio/default_strands.hpp>
#include <restinio/connection_count_limiter.hpp>

namespace restinio
{

namespace details
{

//FIXME: document this!
struct autodetect_request_handler_type {};

//FIXME: document this!
template<
	typename Request_Handler,
	typename User_Data_Factory >
struct actual_request_handler_type_detector
{
//FIXME: there should be static_assert that checks that
//Request_Handler is invocable with incoming_request_t<User_Data_Factory::data_t>.
//	static_assert();

	using request_handler_t = Request_Handler;
};

//FIXME: is this specialization really needed?
#if 0
template<>
struct actual_request_handler_type_detector<
		autodetect_request_handler_type,
		no_user_data_factory_t >
{
	using request_handler_t = default_request_handler_t;
};
#endif

template< typename User_Data_Factory >
struct actual_request_handler_type_detector<
		autodetect_request_handler_type,
		User_Data_Factory >
{
	using request_handler_t = std::function<
			request_handling_status_t(
					incoming_request_handle_t<typename User_Data_Factory::data_t>) >;
};

} /* namespace details */

//
// traits_t
//

template <
		typename Timer_Manager,
		typename Logger,
		typename Request_Handler = details::autodetect_request_handler_type,
		typename Strand = default_strand_t,
		typename Socket = asio_ns::ip::tcp::socket >
struct traits_t
{
	/*!
	 * @brief A type for HTTP methods mapping.
	 *
	 * If RESTinio is used with vanila version of http_parser then
	 * the default value of http_methods_mapper_t is enough. But if a user
	 * uses modified version of http_parser with support of additional,
	 * not-standard HTTP methods then the user should provide its
	 * http_methods_mapper. For example:
	 * \code
	 * // Definition for non-standard HTTP methods.
	 * // Note: values of HTTP_ENCODE and HTTP_DECODE are from modified
	 * // http_parser version.
	 * constexpr const restinio::http_method_id_t http_encode(HTTP_ENCODE, "ENCODE");
	 * constexpr const restinio::http_method_id_t http_decode(HTTP_DECODE, "DECODE");
	 *
	 * // Definition of non-standard http_method_mapper.
	 * struct my_http_method_mapper {
	 * 	inline constexpr restinio::http_method_id_t
	 * 	from_nodejs(int value) noexcept {
	 * 		switch(value) {
	 * 			case HTTP_ENCODE: return http_encode;
	 * 			case HTTP_DECODE: return http_decode;
	 * 			default: return restinio::default_http_methods_t::from_nodejs(value);
	 * 		}
	 * 	}
	 * };
	 * ...
	 * // Make a custom traits with non-standard http_method_mapper.
	 * struct my_server_traits : public restinio::default_traits_t {
	 * 	using http_methods_mapper_t = my_http_method_mapper;
	 * };
	 * \endcode
	 *
	 * @since v.0.5.0
	 */
	using http_methods_mapper_t = default_http_methods_t;

	/*!
	 * @brief A type for connection state listener.
	 *
	 * By default RESTinio doesn't inform about changes with connection state.
	 * But if a user specify its type of connection state listener then
	 * RESTinio will call this listener object when the state of connection
	 * changes.
	 *
	 * An example:
	 * @code
	 * // Definition of user's state listener.
	 * class my_state_listener {
	 * 	...
	 * public:
	 * 	...
	 * 	void state_changed(const restinio::connection_state::notice_t & notice) noexcept {
	 * 		... // some reaction to state change.
	 * 	}
	 * };
	 *
	 * // Definition of custom traits for HTTP server.
	 * struct my_server_traits : public restinio::default_traits_t {
	 * 	using connection_state_listener_t = my_state_listener;
	 * };
	 * @endcode
	 *
	 * @since v.0.5.1
	 */
	using connection_state_listener_t = connection_state::noop_listener_t;

	/*!
	 * @brief A type for IP-blocker.
	 *
	 * By default RESTinio's accepts all incoming connections.
	 * But since v.0.5.1 a user can specify IP-blocker object that
	 * will be called for every new connection. This IP-blocker can
	 * deny or allow a new connection.
	 *
	 * Type of that IP-blocker object is specified by typedef
	 * ip_blocker_t.
	 *
	 * An example:
	 * @code
	 * // Definition of user's IP-blocker.
	 * class my_ip_blocker {
	 * 	...
	 * public:
	 * 	...
	 * 	restinio::ip_blocker::inspection_result_t
	 * 	state_changed(const restinio::ip_blocker::incoming_info_t & info) noexcept {
	 * 		... // some checking for new connection.
	 * 	}
	 * };
	 *
	 * // Definition of custom traits for HTTP server.
	 * struct my_server_traits : public restinio::default_traits_t {
	 * 	using ip_blocker_t = my_ip_blocker;
	 * };
	 * @endcode
	 *
	 * @since v.0.5.1
	 */
	using ip_blocker_t = ip_blocker::noop_ip_blocker_t;

	using timer_manager_t = Timer_Manager;
	using logger_t = Logger;
	using request_handler_t = Request_Handler;
	using strand_t = Strand;
	using stream_socket_t = Socket;

	/*!
	 * @brief A flag that enables or disables the usage of connection count
	 * limiter.
	 *
	 * Since v.0.6.12 RESTinio allows to limit the number of active
	 * parallel connections to a server. But the usage of this limit
	 * should be turned on explicitly. For example:
	 * @code
	 * struct my_traits : public restinio::default_traits_t {
	 * 	static constexpr bool use_connection_count_limiter = true;
	 * };
	 * @endcode
	 * In that case there will be `max_parallel_connections` method
	 * in server_settings_t type. That method should be explicitly
	 * called to set a specific limit (by the default there is no
	 * limit at all):
	 * @code
	 * restinio::server_settings_t<my_traits> settings;
	 * settings.max_parallel_connections(1000u);
	 * @endcode
	 *
	 * @since v.0.6.12
	 */
	static constexpr bool use_connection_count_limiter = false;

	//FIXME: document this!
	using user_data_factory_t = no_user_data_factory_t;
};

//FIXME: document this!
template< typename Traits >
using actual_request_handler_t =
	typename details::actual_request_handler_type_detector<
			typename Traits::request_handler_t,
			typename Traits::user_data_factory_t
		>::request_handler_t;

//FIXME: document this!
template< typename Traits >
using actual_incoming_request_t =
	incoming_request_t< typename Traits::user_data_factory_t::data_t >;

//
// single_thread_traits_t
//

template <
		typename Timer_Manager,
		typename Logger,
		typename Request_Handler = default_request_handler_t >
using single_thread_traits_t =
	traits_t< Timer_Manager, Logger, Request_Handler, noop_strand_t >;

//
// default_traits_t
//

using default_traits_t = traits_t< asio_timer_manager_t, null_logger_t >;

/*!
 * \brief Default traits for single-threaded HTTP-server.
 *
 * Uses default timer manager. And null logger.
 *
 * Usage example:
 * \code
 * struct my_traits : public restinio::default_single_thread_traits_t {
 * 	using logger_t = my_special_single_threaded_logger_type;
 * };
 * \endcode
 *
 * \since
 * v.0.4.0
 */
using default_single_thread_traits_t = single_thread_traits_t<
		asio_timer_manager_t,
		null_logger_t >;

} /* namespace restinio */

