#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <QArgument>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QVariant>

#include "../expandoracommon/parseevent.h"
#include "../global/StringView.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/RoomFieldVariant.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomselection.h"
#include "../proxy/ProxyParserApi.h"
#include "../proxy/telnetfilter.h"
#include "CommandId.h"
#include "CommandQueue.h"
#include "ConnectedRoomFlags.h"
#include "DoorAction.h"
#include "ExitsFlags.h"
#include "PromptFlags.h"

class Coordinate;
class MapData;
class MumeClock;
class ParseEvent;
class Room;
class RoomFieldVariant;
class RoomFilter;

namespace syntax {
class Sublist;
}

class AbstractParser : public QObject
{
protected:
    static const QString nullString;
    static const QByteArray emptyByteArray;

private:
    Q_OBJECT

protected:
    MumeClock *m_mumeClock = nullptr;

private:
    MapData *m_mapData = nullptr;
    ProxyParserApi m_proxy;

public:
    using HelpCallback = std::function<void(const std::string &name)>;
    using ParserCallback
        = std::function<bool(const std::vector<StringView> &matched, StringView args)>;
    struct ParserRecord final
    {
        std::string fullCommand;
        ParserCallback callback;
        HelpCallback help;
    };
    using ParserRecordMap = std::map<std::string, ParserRecord>;

private:
    ParserRecordMap m_specialCommandMap;
    QByteArray m_newLineTerminator;
    const char &prefixChar;

protected:
    QString m_exits = nullString;
    ExitsFlagsType m_exitsFlags;
    PromptFlagsType m_promptFlags;
    ConnectedRoomFlagsType m_connectedRoomFlags;

protected:
    QByteArray m_lastPrompt;
    bool m_compactMode = false;
    bool m_overrideSendPrompt = false;
    CommandQueue m_queue;

private:
    bool m_trollExitMapping = false;
    QTimer m_offlineCommandTimer;

public:
    explicit AbstractParser(MapData *, MumeClock *, ProxyParserApi proxy, QObject *parent = nullptr);
    ~AbstractParser() override;

    void doMove(CommandEnum cmd);
    void sendPromptToUser();

public:
signals:
    // telnet
    void sendToMud(const QByteArray &);
    void sig_sendToUser(const QByteArray &, bool goAhead);
    void sig_mapChanged();
    void sig_graphicsSettingsChanged();
    void releaseAllPaths();

    // used to log
    void log(const QString &, const QString &);

    // for main move/search algorithm
    // CAUTION: This hides virtual bool QObject::event(QEvent*).
    void event(const SigParseEvent &);

    // for map
    void showPath(CommandQueue);
    void newRoomSelection(const SigRoomSelection &rs);

    // for user commands
    void command(const QByteArray &, const Coordinate &);

    // for group manager
    void sendGroupTellEvent(const QByteArray &);
    void sendGroupKickEvent(const QByteArray &);

public slots:
    virtual void parseNewMudInput(const IncomingData &) = 0;
    void parseNewUserInput(const IncomingData &);

    void reset();
    void sendGTellToUser(const QByteArray &);

protected slots:
    void doOfflineCharacterMove();

protected:
    void offlineCharacterMove(CommandEnum direction = CommandEnum::UNKNOWN);
    void sendRoomInfoToUser(const Room *);
    void sendPromptToUser(const Room &r);
    void sendPromptToUser(char light, char terrain);
    void sendPromptToUser(RoomLightEnum lightType, RoomTerrainEnum terrainType);

    void sendRoomExitsInfoToUser(const Room *r);
    const Coordinate getNextPosition();
    const Coordinate getTailPosition();

    // command handling
    void performDoorCommand(ExitDirEnum direction, DoorActionEnum action);
    void genericDoorCommand(QString command, ExitDirEnum direction);
    void nameDoorCommand(const StringView &doorname, ExitDirEnum direction);
    void toggleDoorFlagCommand(DoorFlagEnum flag, ExitDirEnum direction);
    void toggleExitFlagCommand(ExitFlagEnum flag, ExitDirEnum direction);

public:
#define NOP()
#define X_DECLARE_ROOM_FIELD_TOGGLERS(UPPER_CASE, CamelCase, Type) void toggleRoomFlagCommand(Type);
    X_FOREACH_ROOM_FIELD(X_DECLARE_ROOM_FIELD_TOGGLERS, NOP)
#undef X_DECLARE_ROOM_FIELD_TOGGLERS
#undef NOP

public:
    ExitFlags getExitFlags(ExitDirEnum dir) const;
    DirectionalLightEnum getConnectedRoomFlags(ExitDirEnum dir) const;
    void setExitFlags(ExitFlags flag, ExitDirEnum dir);
    void setConnectedRoomFlag(DirectionalLightEnum light, ExitDirEnum dir);

    void printRoomInfo(RoomFields fieldset);
    void printRoomInfo(RoomFieldEnum field);

    void emulateExits(const CommandEnum move);
    QByteArray enhanceExits(const Room *);

    void parseExits();
    void parsePrompt(const QString &prompt);
    virtual bool parseUserCommands(const QString &command);
    static std::string normalizeStringCopy(std::string str);
    static QString normalizeStringCopy(QString str);

    void searchCommand(const RoomFilter &f);
    void dirsCommand(const RoomFilter &f);
    void markCurrentCommand();

private:
    // NOTE: This declaration only exists to avoid the warning
    // about the "event" signal hiding this function function.
    virtual bool event(QEvent *e) final override { return QObject::event(e); }

    bool tryParseGenericDoorCommand(const QString &str);
    void parseSpecialCommand(StringView);
    bool parseSimpleCommand(const QString &str);

    void showDoorCommandHelp();
    void showMumeTime();
    void showHelp();
    void showMapHelp();
    void showGroupHelp();
    void showExitHelp();
    void showRoomSimpleFlagsHelp();
    void showRoomMobFlagsHelp();
    void showRoomLoadFlagsHelp();
    void showMiscHelp();
    void showDoorFlagHelp();
    void showExitFlagHelp();
    void showDoorVariableHelp();
    void showCommandPrefix();
    void showNote();
    void showSyntax(const char *rest);
    void showHelpCommands(bool showAbbreviations);

    void showHeader(const QString &s);

    bool getField(const Coordinate &c,
                  const ExitDirEnum &direction,
                  const ExitFieldVariant &var) const;

    ExitDirEnum tryGetDir(StringView &words);
    bool parseDoorAction(StringView words);
    bool parseDoorFlags(StringView words);
    bool parseExitFlags(StringView words);
    bool parseField(StringView view);
    bool parseMobFlags(StringView view);
    bool parseLoadFlags(StringView view);
    void parseSetCommand(StringView view);
    void parseName(StringView view);
    void parseNoteCmd(StringView view);
    void parseDirections(StringView view);
    void parseSearch(StringView view);
    void parseGroupTell(const StringView &view);
    void parseGroupKick(const StringView &view);

    bool setCommandPrefix(char prefix);
    void setNote(RoomNote note);

    void openVoteURL();
    void doBackCommand();
    void doConnectToHost();
    void doDisconnectFromHost();
    void doRemoveDoorNamesCommand();
    void doMarkCurrentCommand();
    void doSearchCommand(StringView view);
    void doGetDirectionsCommand(StringView view);
    void doGroupLockCommand();
    void toggleTrollMapping();

    void initSpecialCommandMap();
    void addSpecialCommand(const char *s,
                           int minLen,
                           const ParserCallback &callback,
                           const HelpCallback &help);
    bool evalSpecialCommandMap(StringView args);

    void parseHelp(StringView words);
    void parseRoom(StringView input);

    bool parseDoorAction(DoorActionEnum dat, StringView words);
    bool parseDoorFlag(DoorFlagEnum flag, StringView words);
    bool parseExitFlag(ExitFlagEnum flag, StringView words);

public:
    inline void sendToUser(const QByteArray &arr, bool goAhead = false)
    {
        emit sig_sendToUser(arr, goAhead);
    }
    inline void sendToUser(const char *s, bool goAhead = false)
    {
        sendToUser(QByteArray{s}, goAhead);
    }
    inline void sendToUser(const std::string &s, bool goAhead = false)
    {
        sendToUser(QString::fromStdString(s), goAhead);
    }
    inline void sendToUser(const QString &s, bool goAhead = false)
    {
        sendToUser(s.toLatin1(), goAhead);
    }
    void pathChanged() { emit showPath(m_queue); }
    void mapChanged() { emit sig_mapChanged(); }

private:
    void eval(const std::string &name,
              const std::shared_ptr<const syntax::Sublist> &syntax,
              StringView input);
};
