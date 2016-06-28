
#include "ServiceDispatcher.h"
#include "Service.h"
#include "../util/TimeHelper.h"
#include "../util/Log.h"

using namespace sframe;

void MessageQueue::Push(const std::shared_ptr<Message> & msg)
{
	AUTO_LOCK(_lock);

	_buf_write->push_back(msg);
	if (_state == kServiceState_Idle)
	{
		_state = kServiceState_WaitProcess;
		// 调度服务
		ServiceDispatcher::Instance().Dispatch(_related_service->GetServiceId());
	}
}

std::vector<std::shared_ptr<Message>> * MessageQueue::PopAll()
{
	AUTO_LOCK(_lock);
	
	assert(_state == kServiceState_WaitProcess);
	auto temp = _buf_write;
	_buf_write = _buf_read;
	_buf_read = temp;
	_state = kServiceState_Processing;

	return _buf_read;
}

void MessageQueue::EndProcess()
{
	AUTO_LOCK(_lock);

	assert(_state == kServiceState_Processing);
	_buf_read->clear();

	// 若又有新的消息写入
	if (!_buf_write->empty())
	{
		_state = kServiceState_WaitProcess;
		// 调度服务
		ServiceDispatcher::Instance().Dispatch(_related_service->GetServiceId());
	}
	else
	{
		_state = kServiceState_Idle;
	}
}

// 处理
void Service::Process()
{
	auto msgs = _msg_queue.PopAll();
	assert(msgs && !msgs->empty());

	for (auto & msg : *msgs)
	{
		MessageType msg_type = msg->GetType();
		switch (msg_type)
		{
			case sframe::kMsgType_CycleMessage:
			{
				auto cycle_msg = std::static_pointer_cast<CycleMessage>(msg);
				_cur_time = TimeHelper::GetEpochMilliseconds();
				this->OnCycleTimer();
				cycle_msg->Unlock();
			}
			break;
			
			case sframe::kMsgType_InsideServiceMessage:
			{
				auto service_msg = std::static_pointer_cast<ServiceMessage>(msg);
				assert(service_msg->dest_sid == GetServiceId());
				_last_sid = service_msg->src_sid;
				InsideServiceMessageDecoder decoder(service_msg.get());
				_inside_delegate_mgr.Call(service_msg->msg_id, decoder);
			}
			break;

			case sframe::kMsgType_NetServiceMessage:
			{
				auto net_service_msg = std::static_pointer_cast<NetServiceMessage>(msg);
				assert(net_service_msg->dest_sid == GetServiceId());
				_last_sid = net_service_msg->src_sid;
				NetServiceMessageDecoder decoder(net_service_msg.get());
				_net_delegate_mgr.Call(net_service_msg->msg_id, decoder);
			}
			break;

			case sframe::kMsgType_ProxyServiceMessage:
			{
				auto proxy_service_msg = std::static_pointer_cast<ProxyServiceMessage>(msg);
				OnProxyServiceMessage(proxy_service_msg);
			}
			break;

			case sframe::kMsgType_DestroyServiceMessage:
			{
				this->OnDestroy();
			}
			break;

			case sframe::kMsgType_ServiceLostMessage:
			{
				auto lost_msg = std::static_pointer_cast<ServiceLostMessage>(msg);
				this->OnServiceLost(lost_msg->service);
			}
			break;

			case sframe::kMsgType_NewConnectionMessage:
			{
				auto new_conn_msg = std::static_pointer_cast<NewConnectionMessage>(msg);
				this->OnNewConnection(new_conn_msg->GetListenAddress(), new_conn_msg->GetSocket());
			}
			break;
		}
	}

	_msg_queue.EndProcess();
}

// 等待销毁完毕
void Service::WaitDestroyComplete()
{
	LOG_INFO << "wait service " << GetServiceId() << " destroy" << std::endl;

	int64_t start_time = TimeHelper::GetEpochMilliseconds();
	bool succ = true;

	while (!IsDestroyCompleted())
	{
		TimeHelper::ThreadSleep(50);

		if (kDefaultMaxWaitDestroyTime >= 0)
		{
			int64_t cur_time = TimeHelper::GetEpochMilliseconds();
			if (cur_time - start_time >= kDefaultMaxWaitDestroyTime)
			{
				succ = false;
				break;
			}
		}
	}

	LOG_INFO << "wait service " << GetServiceId() << " destroy|" << (succ ? "succ" : "timeout") << std::endl;
}