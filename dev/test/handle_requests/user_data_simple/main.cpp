/*
	restinio
*/

#include <catch2/catch.hpp>

#include <restinio/all.hpp>

#include <test/common/utest_logger.hpp>
#include <test/common/pub.hpp>

namespace test
{

class test_user_data_factory_t
{
	std::atomic<int> m_index_counter{0};

public:
	struct data_t
	{
		int m_index;
	};

	void
	make_within( restinio::user_data_buffer_t<data_t> buffer ) noexcept
	{
		new(buffer.get()) data_t{ ++m_index_counter };
	}

	RESTINIO_NODISCARD
	int
	current_value() noexcept
	{
		return m_index_counter.load( std::memory_order_acquire );
	}
};

struct test_traits_t : public restinio::traits_t<
	restinio::asio_timer_manager_t, utest_logger_t >
{
	using user_data_factory_t = test_user_data_factory_t;
};

} /* namespace test */

TEST_CASE( "remote_endpoint extraction" , "[remote_endpoint]" )
{
	using namespace test;

	using http_server_t = restinio::http_server_t< test_traits_t >;

	std::string endpoint_value;
	int index_value;

	auto user_data_factory = std::make_shared< test_user_data_factory_t >();

	http_server_t http_server{
		restinio::own_io_context(),
		[&endpoint_value, &index_value, user_data_factory]( auto & settings ){
			settings
				.port( utest_default_port() )
				.address( "127.0.0.1" )
				.user_data_factory( user_data_factory )
				.request_handler(
					[&endpoint_value, &index_value]( auto req ){
						endpoint_value = fmt::format( "{}", req->remote_endpoint() );
						index_value = req->user_data().m_index;

						req->create_response()
							.append_header( "Server", "RESTinio utest server" )
							.append_header_date_field()
							.append_header( "Content-Type", "text/plain; charset=utf-8" )
							.set_body(
								restinio::const_buffer( req->header().method().c_str() ) )
							.done();

						return restinio::request_accepted();
					} );
		} };

	other_work_thread_for_server_t<http_server_t> other_thread(http_server);
	other_thread.run();

	std::string response;
	const char * request_str =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1\r\n"
		"User-Agent: unit-test\r\n"
		"Accept: */*\r\n"
		"Connection: close\r\n"
		"\r\n";

	REQUIRE_NOTHROW( response = do_request( request_str ) );

	REQUIRE_THAT( response, Catch::Matchers::EndsWith( "GET" ) );

	other_thread.stop_and_join();

	REQUIRE( !endpoint_value.empty() );
	REQUIRE( 0 != user_data_factory->current_value() );
	REQUIRE( 1 == index_value );
}

