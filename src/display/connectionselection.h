#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h" /* Coordinate2f */
#include "../global/Array.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"

class MapFrontend;
class Room;
class RoomAdmin;
struct RoomId;

struct MouseSel final
{
    Coordinate2f pos;
    int layer = 0;

    MouseSel() = default;
    explicit MouseSel(const Coordinate2f &pos, const int layer)
        : pos{pos}
        , layer{layer}
    {}

    Coordinate getCoordinate() const { return Coordinate{pos.truncate(), layer}; }
    Coordinate getScaledCoordinate(const float xy_scale) const
    {
        return Coordinate{(pos * xy_scale).truncate(), layer};
    }

    glm::vec3 to_vec3() const { return glm::vec3{pos.to_vec2(), static_cast<float>(layer)}; }
};

class ConnectionSelection final : public RoomRecipient,
                                  public std::enable_shared_from_this<ConnectionSelection>
{
public:
    struct ConnectionDescriptor final
    {
        const Room *room = nullptr;
        ExitDirEnum direction = ExitDirEnum::NONE;

        static bool isTwoWay(const ConnectionDescriptor &first, const ConnectionDescriptor &second);
        static bool isOneWay(const ConnectionDescriptor &first, const ConnectionDescriptor &second);
        static bool isCompleteExisting(const ConnectionDescriptor &first,
                                       const ConnectionDescriptor &second);
        static bool isCompleteNew(const ConnectionDescriptor &first,
                                  const ConnectionDescriptor &second);
    };

private:
    struct this_is_private final
    {
        explicit this_is_private(int) {}
    };

public:
    static std::shared_ptr<ConnectionSelection> alloc(MapFrontend *mf, const MouseSel &sel)
    {
        return std::make_shared<ConnectionSelection>(this_is_private{0}, mf, sel);
    }
    static std::shared_ptr<ConnectionSelection> alloc()
    {
        return std::make_shared<ConnectionSelection>(this_is_private{0});
    }

    explicit ConnectionSelection(this_is_private, MapFrontend *mf, const MouseSel &sel);
    explicit ConnectionSelection(this_is_private);
    ~ConnectionSelection() override;
    DELETE_CTORS_AND_ASSIGN_OPS(ConnectionSelection);

    void setFirst(MapFrontend *mf, RoomId RoomId, ExitDirEnum dir);
    void setSecond(MapFrontend *mf, RoomId RoomId, ExitDirEnum dir);
    void setSecond(MapFrontend *mf, const MouseSel &sel);
    void removeSecond();

    ConnectionDescriptor getFirst() const;
    ConnectionDescriptor getSecond() const;

    // Valid just means the pointers aren't null.
    bool isValid() const;
    bool isFirstValid() const { return m_connectionDescriptor[0].room != nullptr; }
    bool isSecondValid() const { return m_connectionDescriptor[1].room != nullptr; }

    void receiveRoom(RoomAdmin *admin, const Room *aRoom) override;

    // Complete means it actually describes a useful oneway or twoway exit.
    bool isCompleteExisting() const
    {
        return isValid() && ConnectionDescriptor::isCompleteExisting(getFirst(), getSecond());
    }
    bool isCompleteNew() const
    {
        return isValid() && ConnectionDescriptor::isCompleteNew(getFirst(), getSecond());
    }

private:
    static ExitDirEnum computeDirection(const Coordinate2f &mouse_f);

    // REVISIT: give these enum names?
    MMapper::Array<ConnectionDescriptor, 2> m_connectionDescriptor;

    bool m_first = true;
    RoomAdmin *m_admin = nullptr;
};
