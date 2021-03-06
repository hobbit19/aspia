//
// Aspia Project
// Copyright (C) 2019 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "host/host_ui_process.h"
#include "base/password_generator.h"
#include "base/qt_logging.h"
#include "common/message_serialization.h"
#include "host/win/host_process.h"
#include "ipc/ipc_channel.h"
#include "net/adapter_enumerator.h"
#include "proto/host.pb.h"

#include <QCoreApplication>

namespace host {

UiProcess::UiProcess(ipc::Channel* channel, QObject* parent)
    : QObject(parent),
      channel_(channel)
{
    channel_->setParent(this);
}

UiProcess::~UiProcess() = default;

// static
void UiProcess::create(base::win::SessionId session_id)
{
    LOG(LS_INFO) << "Starting the UI (SID: " << session_id << ")";

    HostProcess process;

    process.setAccount(HostProcess::User);
    process.setSessionId(session_id);
    process.setProgram(QCoreApplication::applicationDirPath() + QStringLiteral("/aspia_host.exe"));
    process.setArguments(QStringList() << QStringLiteral("--hidden"));

    HostProcess::ErrorCode error_code = process.start();
    if (error_code != HostProcess::ErrorCode::NoError)
    {
        LOG(LS_ERROR) << "UI process is not started";
    }
    else
    {
        LOG(LS_INFO) << "UI process is started successfully";
    }
}

base::win::SessionId UiProcess::sessionId() const
{
    if (!channel_)
        return base::win::kInvalidSessionId;

    return channel_->clientSessionId();
}

void UiProcess::setConnectEvent(const proto::host::ConnectEvent& event)
{
    proto::host::ServiceToUi message;
    message.mutable_connect_event()->CopyFrom(event);
    channel_->send(common::serializeMessage(message));
}

void UiProcess::setDisconnectEvent(const std::string& uuid)
{
    proto::host::ServiceToUi message;
    message.mutable_disconnect_event()->set_uuid(uuid);
    channel_->send(common::serializeMessage(message));
}

bool UiProcess::start()
{
    if (state_ != State::STOPPED)
    {
        DLOG(LS_ERROR) << "Attempting to start a process that is already running";
        return false;
    }

    process_ = new base::win::Process(channel_->clientProcessId(), this);
    if (!process_->isValid())
    {
        LOG(LS_ERROR) << "Unable to open UI process";
        return false;
    }

    connect(process_, &base::win::Process::finished,
            this, &UiProcess::onProcessFinished,
            Qt::QueuedConnection);

    connect(channel_, &ipc::Channel::messageReceived, this, &UiProcess::onMessageReceived);
    connect(channel_, &ipc::Channel::disconnected, this, &UiProcess::stop, Qt::QueuedConnection);

    sendCredentials(proto::host::CredentialsRequest::REFRESH |
                    proto::host::CredentialsRequest::NEW_PASSWORD);

    state_ = State::STARTED;
    channel_->start();
    return true;
}

void UiProcess::stop()
{
    if (state_ == State::STOPPED)
        return;

    state_ = State::STOPPED;

    if (channel_)
        channel_->stop();

    LOG(LS_INFO) << "UI is stopped";
    emit finished();
}

void UiProcess::onProcessFinished(int exit_code)
{
    LOG(LS_INFO) << "UI process finished with code: " << exit_code;
    stop();
}

void UiProcess::onMessageReceived(const QByteArray& buffer)
{
    proto::host::UiToService message;

    if (!common::parseMessage(buffer, message))
    {
        LOG(LS_ERROR) << "Invalid message from UI";
        stop();
        return;
    }

    if (message.has_credentials_request())
    {
        sendCredentials(message.credentials_request().flags());
    }
    else if (message.has_kill_session())
    {
        emit killSession(message.kill_session().uuid());
    }
    else
    {
        LOG(LS_WARNING) << "Unhandled message from UI";
    }
}

void UiProcess::sendCredentials(uint32_t flags)
{
    if (!flags)
    {
        LOG(LS_ERROR) << "Invalid flag value";
        return;
    }

    proto::host::ServiceToUi message;

    if ((flags & proto::host::CredentialsRequest::NEW_PASSWORD) || session_password_.empty())
    {
        base::PasswordGenerator generator;

        // TODO: Get password parameters from settings.
        generator.setCharacters(base::PasswordGenerator::UPPER_CASE |
                                base::PasswordGenerator::DIGITS);
        generator.setLength(5);

        session_password_ = generator.result();

        emit userChanged(channel_->clientSessionId(), session_password_);
    }

    message.mutable_credentials()->set_username("#" + std::to_string(sessionId()));
    message.mutable_credentials()->set_password(session_password_);

    for (net::AdapterEnumerator adapters; !adapters.isAtEnd(); adapters.advance())
    {
        for (net::AdapterEnumerator::IpAddressEnumerator addresses(adapters);
             !addresses.isAtEnd(); addresses.advance())
        {
            std::string ip = addresses.address();

            if (ip != "127.0.0.1")
                message.mutable_credentials()->add_ip(ip);
        }
    }

    channel_->send(common::serializeMessage(message));
}

} // namespace host
