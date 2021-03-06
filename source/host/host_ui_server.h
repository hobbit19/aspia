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

#ifndef HOST__HOST_UI_SERVER_H
#define HOST__HOST_UI_SERVER_H

#include "base/macros_magic.h"
#include "base/win/session_id.h"
#include "base/win/session_status.h"
#include "net/srp_user.h"
#include "proto/host.pb.h"

#include <QObject>
#include <QPointer>

namespace ipc {
class Channel;
class Server;
} // namespace ipc

namespace host {

class UiProcess;

class UiServer : public QObject
{
    Q_OBJECT

public:
    explicit UiServer(QObject* parent = nullptr);
    ~UiServer();

    using UserList = std::map<base::win::SessionId, net::SrpUser>;

    enum class State { STOPPED, STOPPING, STARTED };
    enum class EventType { CONNECTED, DISCONNECTED };

    bool start();
    void stop();

    State state() const { return state_; }

    bool hasUiForSession(base::win::SessionId session_id) const;

    void setSessionEvent(base::win::SessionStatus status, base::win::SessionId session_id);
    void setConnectEvent(base::win::SessionId session_id, const proto::host::ConnectEvent& event);
    void setDisconnectEvent(base::win::SessionId session_id, const std::string& uuid);

    const UserList& userList() const { return user_list_; }

signals:
    void processEvent(EventType event, base::win::SessionId session_id);
    void userListChanged();
    void killSession(const std::string& uuid);
    void finished();

private slots:
    void onChannelConnected(ipc::Channel* channel);
    void onProcessFinished();
    void onUserChanged(base::win::SessionId session_id, const std::string& password);

private:
    State state_ = State::STOPPED;

    // IPC server accepts incoming connections from UI processes.
    QPointer<ipc::Server> server_;

    // List of connected UI processes.
    std::list<UiProcess*> process_list_;

    UserList user_list_;

    DISALLOW_COPY_AND_ASSIGN(UiServer);
};

} // namespace host

#endif // HOST__HOST_UI_SERVER_H
