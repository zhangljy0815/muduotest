#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>
#include <utility>

#include <muduo/base/Thread.h>
#include <muduo/base/Atomic.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>

using namespace muduo;
using namespace muduo::net;
int numThreads = 0;

class  EchoServer
{
public:
	EchoServer(EventLoop* loop, const InetAddress& listenAddr)
		: 	m_server(loop, listenAddr, "EchoServer"),
			m_oldCounter(0),
			m_startTime(Timestamp::now())
	{
		m_server.setConnectionCallback(boost::bind(&EchoServer::onConnection, this, _1));
		m_server.setMessageCallback(boost::bind(&EchoServer::onMessage,this, _1, _2, _3));
		m_server.setThreadNum(numThreads);
		loop->runEvery(3.0, boost::bind(&EchoServer::printThroughput, this));
	}
	
	void start()
	{
		LOG_INFO << "starting " << numThreads << " thread. ";
		m_server.start();
	}
	
private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
			<< conn->localAddress().toIPPort() << " is " << 
			(conn->connected() ? "UP" : "DOWN");
		conn->setTcpNoDelay();
	}
	
	void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
	{
		size_t len = buf->readableBytes();
		m_transferred.addAndGet(len);
		m_receivedMessages.incrementAndGet();
		conn->send(buf);
	}
	
	void printThroughput()
	{
		Timestamp endTime = Timestamp::now();
		int64_t newCounter = m_transferred.get();
		int64_t bytes = newCounter - oldCounter;
		int64_t msgs = m_receivedMessages.GetAndSet();
		double time = timeDifference(endTime, m_startTime);
		printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
			static_cast<double>(bytes)/time/1024/1024,
			static_cast<double>(msgs)/time/1024,
			static_cast<double>(bytes)/static_cast<double>(msgs));
	}
	TcpServer m_server;
	Timestamp m_startTime;
	int64_t m_oldCounter;
	AtomicInt64 m_transferred;
	AtomicInt64 m_receivedMessages;
	
};

int main(int argc, char **argv)
{
	LOG_INFO << "pid = " << getpid() << "tid = " << CurrentThread::tid();
	
	if(argc > 1)
	{
		numThreads = atoi(argv[1]);
	}
	
	EventLoop loop;
	InetAddress listenAddress(2007);
	
	EchoServer server(&loop, listenAddress);
	server.start();
	
	return 0;
}

