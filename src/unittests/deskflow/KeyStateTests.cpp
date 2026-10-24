/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyStateTests.h"
#include "base/EventQueue.h"
#include "deskflow/KeyMap.h"
#include "common/JISKeyboard.h"

#include "MockEventQueue.h"
#include "MockKeyMap.h"
#include "MockKeyState.h"

#include <algorithm>

namespace {

//! KeyState that records the keystrokes it is asked to synthesize.
class RecordingKeyState : public KeyState
{
public:
  RecordingKeyState(IEventQueue *events, deskflow::KeyMap &keyMap, std::vector<std::string> layouts, bool langSync)
      : KeyState(events, keyMap, std::move(layouts), langSync)
  {
  }

  int32_t pollActiveGroup() const override
  {
    return m_activeGroup;
  }
  KeyModifierMask pollActiveModifiers() const override
  {
    return 0;
  }
  bool fakeCtrlAltDel() override
  {
    return false;
  }
  void getKeyMap(deskflow::KeyMap &) override
  {
  }
  bool fakeMediaKey(KeyID) override
  {
    return false;
  }
  void pollPressedKeys(KeyButtonSet &) const override
  {
  }
  void fakeKey(const Keystroke &keystroke) override
  {
    m_faked.push_back(keystroke);
  }

  int countStrokes(Keystroke::KeyType type) const
  {
    return static_cast<int>(std::count_if(m_faked.begin(), m_faked.end(), [type](const Keystroke &k) {
      return k.m_type == type;
    }));
  }

  int32_t m_activeGroup = 0;
  std::vector<Keystroke> m_faked;
};

//! Two groups on one button: a latin key in group 0 ("en"), a thai key in group 1 ("th").
void buildTwoGroupKeyMap(deskflow::KeyMap &keyMap, KeyID enKey, KeyID thKey, KeyButton button)
{
  deskflow::KeyMap::KeyItem item;
  item.m_button = button;

  item.m_id = enKey;
  item.m_group = 0;
  keyMap.addKeyEntry(item);

  item.m_id = thKey;
  item.m_group = 1;
  keyMap.addKeyEntry(item);

  keyMap.finish();
}

constexpr KeyID kLatinA = 'a';
constexpr KeyID kThaiFoFan = 0x0e1f; // ฟ, the same physical key as 'a' on a thai layout
constexpr KeyButton kSharedButton = 1;

} // namespace

void KeyStateTests::initTestCase()
{
  m_arch.init();
}

void KeyStateTests::keyDown()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  keyState.onKey(1, true, KeyModifierAlt);

  QVERIFY(keyState.getKeyState(1));
}

void KeyStateTests::keyUp()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);
  QVERIFY(!keyState.getKeyState(1));
}

void KeyStateTests::invalidKey()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(0, true, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(0));
}

void KeyStateTests::onKey_aKeyDown_keyStateOne()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(1, true, KeyModifierAlt);

  QVERIFY(keyState.getKeyState(1));
}

void KeyStateTests::onKey_aKeyUp_keyStateZero()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(1, false, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(1));
}

void KeyStateTests::onKey_invalidKey_keyStateZero()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(0, true, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(0));
}

void KeyStateTests::updateKeyState_pollDoesNothing_keyNotSet()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.updateKeyState();

  QVERIFY(!keyState.isKeyDown(1));
}

void KeyStateTests::updateKeyState_activeModifiers_maskNotSet()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.updateKeyState();

  QCOMPARE(0, keyState.getActiveModifiers());
}

void KeyStateTests::fakeKeyRepeat_invalidKey_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.fakeKeyRepeat(0, 0, 0, 0, "en"));
}

void KeyStateTests::fakeKeyUp_buttonNotDown_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.fakeKeyUp(0));
}

void KeyStateTests::isKeyDown_noKeysDown_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.isKeyDown(1));
}

void KeyStateTests::isKeyDown_keyDown_retrunsTrue()
{
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  deskflow::KeyMap::KeyItem key;
  key.m_button = 1;
  keyState.fakeKeyDown(1, 0, 1, "en");

  QVERIFY(keyState.isKeyDown(1));
}

void KeyStateTests::updateKeyState_pollInsertsSingleKey_keyIsDown()
{
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  deskflow::KeyMap::KeyItem key;
  key.m_button = 1;
  keyState.fakeKeyDown(1, 0, 1, "en");

  keyState.updateKeyState();
  QVERIFY(keyState.isKeyDown(1));
}

void KeyStateTests::fakeKeyDown_langSyncEnabled_switchesToServerGroup()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);

  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, true);
  keyState.fakeKeyDown(kThaiFoFan, 0, kSharedButton, "th");

  QCOMPARE(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 1);
  const auto group = std::find_if(keyState.m_faked.begin(), keyState.m_faked.end(), [](const auto &k) {
    return k.m_type == deskflow::KeyMap::Keystroke::KeyType::Group;
  });
  QCOMPARE(group->m_data.m_group.m_group, 1);
}

void KeyStateTests::fakeKeyDown_langSyncDisabled_keepsLocalGroup()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);

  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, false);
  keyState.fakeKeyDown(kThaiFoFan, 0, kSharedButton, "th");

  // the local layout must be left alone, but the key itself is still pressed by position
  QCOMPARE(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 0);
  QVERIFY(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Button) > 0);
  QCOMPARE(keyState.m_faked.front().m_data.m_button.m_button, kSharedButton);
}

void KeyStateTests::fixedJapaneseLayout_mapsPressAndRepeatToJISPositions()
{
  for (const std::string actual : {"en", "ja"}) {
    MockEventQueue events;
    deskflow::KeyMap map;
    deskflow::KeyMap::KeyItem item;
    item.m_id = '(';
    item.m_group = 0;
    item.m_button = 9; // US Shift+9
    map.addKeyEntry(item);
    item.m_group = 1;
    item.m_button = 8; // JIS Shift+8
    map.addKeyEntry(item);
    map.finish();
    RecordingKeyState state(&events, map, {"en", "ja"}, true);
    state.m_activeGroup = 0; // Foreground app was left on English.
    const auto language = deskflow::jis::language(actual, true);
    state.fakeKeyDown('(', 0, 55, language);
    QCOMPARE(state.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 1);
    QVERIFY(state.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Button) > 0);
    for (const auto &stroke : state.m_faked) {
      if (stroke.m_type == deskflow::KeyMap::Keystroke::KeyType::Button) {
        QCOMPARE(stroke.m_data.m_button.m_button, KeyButton(8));
      }
    }
    state.m_faked.clear();
    QVERIFY(state.fakeKeyRepeat('(', 0, 1, 55, language));
    for (const auto &stroke : state.m_faked) {
      if (stroke.m_type == deskflow::KeyMap::Keystroke::KeyType::Button) {
        QCOMPARE(stroke.m_data.m_button.m_button, KeyButton(8));
      }
    }
  }
}

void KeyStateTests::fixedJapaneseLayout_preservesActiveJapaneseIME()
{
  MockEventQueue events;
  deskflow::KeyMap map;
  for (int group = 0; group < 3; ++group) {
    deskflow::KeyMap::KeyItem item;
    item.m_id = '(';
    item.m_group = group;
    item.m_button = (group == 0) ? 9 : 8;
    map.addKeyEntry(item);
  }
  map.finish();
  RecordingKeyState state(&events, map, {"en", "ja", "ja"}, true);
  state.m_activeGroup = 2; // ATOK; group 1 is another Japanese IME.
  state.fakeKeyDown('(', 0, 55, deskflow::jis::language("en", true));
  QCOMPARE(state.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 0);
  QVERIFY(state.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Button) > 0);
}

QTEST_MAIN(KeyStateTests)
