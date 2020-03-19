/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qshortcut.h"
#include "qshortcut_p.h"

#include <qevent.h>
#include <qguiapplication.h>
#include <qwindow.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformmenu.h>

QT_BEGIN_NAMESPACE

#define QAPP_CHECK(functionName) \
    if (Q_UNLIKELY(!qApp)) {                                            \
        qWarning("QShortcut: Initialize QGuiApplication before calling '" functionName "'."); \
        return; \
    }

/*!
    \class QShortcut
    \brief The QShortcut class is used to create keyboard shortcuts.

    \ingroup events
    \inmodule QtGui

    The QShortcut class provides a way of connecting keyboard
    shortcuts to Qt's \l{signals and slots} mechanism, so that
    objects can be informed when a shortcut is executed. The shortcut
    can be set up to contain all the key presses necessary to
    describe a keyboard shortcut, including the states of modifier
    keys such as \uicontrol Shift, \uicontrol Ctrl, and \uicontrol Alt.

    \target mnemonic

    In widget applications, certain widgets can use '&' in front of a character.
    This will automatically create a mnemonic (a shortcut) for that character,
    e.g. "E&xit" will create the shortcut \uicontrol Alt+X (use '&&'
    to display an actual ampersand). The widget might consume and perform
    an action on a given shortcut. On X11 the ampersand will not be
    shown and the character will be underlined. On Windows, shortcuts
    are normally not displayed until the user presses the \uicontrol Alt
    key, but this is a setting the user can change. On Mac, shortcuts
    are disabled by default. Call \l qt_set_sequence_auto_mnemonic() to
    enable them. However, because mnemonic shortcuts do not fit in
    with Aqua's guidelines, Qt will not show the shortcut character
    underlined.

    For applications that use menus, it may be more convenient to
    use the convenience functions provided in the QMenu class to
    assign keyboard shortcuts to menu items as they are created.
    Alternatively, shortcuts may be associated with other types of
    actions in the QAction class.

    The simplest way to create a shortcut for a particular widget is
    to construct the shortcut with a key sequence. For example:

    \snippet code/src_gui_kernel_qshortcut.cpp 0

    When the user types the \l{QKeySequence}{key sequence}
    for a given shortcut, the shortcut's activated() signal is
    emitted. (In the case of ambiguity, the activatedAmbiguously()
    signal is emitted.) A shortcut is "listened for" by Qt's event
    loop when the shortcut's parent widget is receiving events.

    A shortcut's key sequence can be set with setKey() and retrieved
    with key(). A shortcut can be enabled or disabled with
    setEnabled(), and can have "What's This?" help text set with
    setWhatsThis().

    \sa QShortcutEvent, QKeySequence, QAction
*/

/*!
    \fn void QShortcut::activated()

    This signal is emitted when the user types the shortcut's key
    sequence.

    \sa activatedAmbiguously()
*/

/*!
    \fn void QShortcut::activatedAmbiguously()

    When a key sequence is being typed at the keyboard, it is said to
    be ambiguous as long as it matches the start of more than one
    shortcut.

    When a shortcut's key sequence is completed,
    activatedAmbiguously() is emitted if the key sequence is still
    ambiguous (i.e., it is the start of one or more other shortcuts).
    The activated() signal is not emitted in this case.

    \sa activated()
*/

static bool simpleContextMatcher(QObject *object, Qt::ShortcutContext context)
{
    auto guiShortcut = qobject_cast<QShortcut *>(object);
    if (QGuiApplication::applicationState() != Qt::ApplicationActive || guiShortcut == nullptr)
        return false;
    if (context == Qt::ApplicationShortcut)
        return true;
    auto focusWindow = QGuiApplication::focusWindow();
    if (!focusWindow)
        return false;
    auto window = qobject_cast<const QWindow *>(guiShortcut->parent());
    if (!window)
        return false;
    if (focusWindow == window && focusWindow->isTopLevel())
        return context == Qt::WindowShortcut || context == Qt::WidgetWithChildrenShortcut;
    return focusWindow->isAncestorOf(window, QWindow::ExcludeTransients);
}

QShortcutMap::ContextMatcher QShortcutPrivate::contextMatcher() const
{
    return simpleContextMatcher;
}

void QShortcutPrivate::redoGrab(QShortcutMap &map)
{
    Q_Q(QShortcut);
    if (Q_UNLIKELY(!parent)) {
        qWarning("QShortcut: No window parent defined");
        return;
    }

    if (sc_id)
        map.removeShortcut(sc_id, q);
    if (sc_sequence.isEmpty())
        return;
    sc_id = map.addShortcut(q, sc_sequence, sc_context, contextMatcher());
    if (!sc_enabled)
        map.setShortcutEnabled(false, sc_id, q);
    if (!sc_autorepeat)
        map.setShortcutAutoRepeat(false, sc_id, q);
}

QShortcutPrivate *QGuiApplicationPrivate::createShortcutPrivate() const
{
    return new QShortcutPrivate;
}

/*!
    Constructs a QShortcut object for the \a parent, which should be a
    QWindow or a QWidget.

    Since no shortcut key sequence is specified, the shortcut will not emit any
    signals.

    \sa setKey()
*/
QShortcut::QShortcut(QObject *parent)
    : QObject(*QGuiApplicationPrivate::instance()->createShortcutPrivate(), parent)
{
    Q_ASSERT(parent != nullptr);
}

/*!
    Constructs a QShortcut object for the \a parent, which should be a
    QWindow or a QWidget.

    The shortcut operates on its parent, listening for \l{QShortcutEvent}s that
    match the \a key sequence. Depending on the ambiguity of the event, the
    shortcut will call the \a member function, or the \a ambiguousMember function,
    if the key press was in the shortcut's \a context.
*/
QShortcut::QShortcut(const QKeySequence &key, QObject *parent,
                           const char *member, const char *ambiguousMember,
                           Qt::ShortcutContext context)
    : QShortcut(parent)
{
    Q_D(QShortcut);
    d->sc_context = context;
    d->sc_sequence = key;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    if (member)
        connect(this, SIGNAL(activated()), parent, member);
    if (ambiguousMember)
        connect(this, SIGNAL(activatedAmbiguously()), parent, ambiguousMember);
}


/*!
    \fn template<typename Functor>
        QShortcut(const QKeySequence &key, QObject *parent,
                  Functor functor,
                  Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    \since 5.15
    \overload

    This is a QShortcut convenience constructor which connects the shortcut's
    \l{QShortcut::activated()}{activated()} signal to the \a functor.
*/
/*!
    \fn template<typename Functor>
        QShortcut(const QKeySequence &key, QObject *parent,
                  const QObject *context, Functor functor,
                  Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    \since 5.15
    \overload

    This is a QShortcut convenience constructor which connects the shortcut's
    \l{QShortcut::activated()}{activated()} signal to the \a functor.

    The \a functor can be a pointer to a member function of the \a context object.

    If the \a context object is destroyed, the \a functor will not be called.
*/
/*!
    \fn template<typename Functor, typename FunctorAmbiguous>
        QShortcut(const QKeySequence &key, QObject *parent,
                  const QObject *context1, Functor functor,
                  FunctorAmbiguous functorAmbiguous,
                  Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    \since 5.15
    \overload

    This is a QShortcut convenience constructor which connects the shortcut's
    \l{QShortcut::activated()}{activated()} signal to the \a functor and
    \l{QShortcut::activatedAmbiguously()}{activatedAmbiguously()}
    signal to the \a FunctorAmbiguous.

    The \a functor and \a FunctorAmbiguous can be a pointer to a member
    function of the \a context object.

    If the \a context object is destroyed, the \a functor and
    \a FunctorAmbiguous will not be called.
*/
/*!
    \fn template<typename Functor, typename FunctorAmbiguous>
        QShortcut(const QKeySequence &key, QObject *parent,
                  const QObject *context1, Functor functor,
                  const QObject *context2, FunctorAmbiguous functorAmbiguous,
                  Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    \since 5.15
    \overload

    This is a QShortcut convenience constructor which connects the shortcut's
    \l{QShortcut::activated()}{activated()} signal to the \a functor and
    \l{QShortcut::activatedAmbiguously()}{activatedAmbiguously()}
    signal to the \a FunctorAmbiguous.

    The \a functor can be a pointer to a member function of the
    \a context1 object.
    The \a FunctorAmbiguous can be a pointer to a member function of the
    \a context2 object.

    If the \a context1 object is destroyed, the \a functor will not be called.
    If the \a context2 object is destroyed, the \a FunctorAmbiguous
    will not be called.
*/

/*!
    Destroys the shortcut.
*/
QShortcut::~QShortcut()
{
    Q_D(QShortcut);
    if (qApp)
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(d->sc_id, this);
}

/*!
    \property QShortcut::key
    \brief the shortcut's key sequence

    This is a key sequence with an optional combination of Shift, Ctrl,
    and Alt. The key sequence may be supplied in a number of ways:

    \snippet code/src_gui_kernel_qshortcut.cpp 1

    By default, this property contains an empty key sequence.
*/
void QShortcut::setKey(const QKeySequence &key)
{
    Q_D(QShortcut);
    if (d->sc_sequence == key)
        return;
    QAPP_CHECK("setKey");
    d->sc_sequence = key;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
}

QKeySequence QShortcut::key() const
{
    Q_D(const QShortcut);
    return d->sc_sequence;
}

/*!
    \property QShortcut::enabled
    \brief whether the shortcut is enabled

    An enabled shortcut emits the activated() or activatedAmbiguously()
    signal when a QShortcutEvent occurs that matches the shortcut's
    key() sequence.

    If the application is in \c WhatsThis mode the shortcut will not emit
    the signals, but will show the "What's This?" text instead.

    By default, this property is \c true.

    \sa whatsThis
*/
void QShortcut::setEnabled(bool enable)
{
    Q_D(QShortcut);
    if (d->sc_enabled == enable)
        return;
    QAPP_CHECK("setEnabled");
    d->sc_enabled = enable;
    QGuiApplicationPrivate::instance()->shortcutMap.setShortcutEnabled(enable, d->sc_id, this);
}

bool QShortcut::isEnabled() const
{
    Q_D(const QShortcut);
    return d->sc_enabled;
}

/*!
    \property QShortcut::context
    \brief the context in which the shortcut is valid

    A shortcut's context decides in which circumstances a shortcut is
    allowed to be triggered. The normal context is Qt::WindowShortcut,
    which allows the shortcut to trigger if the parent (the widget
    containing the shortcut) is a subwidget of the active top-level
    window.

    By default, this property is set to Qt::WindowShortcut.
*/
void QShortcut::setContext(Qt::ShortcutContext context)
{
    Q_D(QShortcut);
    if (d->sc_context == context)
        return;
    QAPP_CHECK("setContext");
    d->sc_context = context;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
}

Qt::ShortcutContext QShortcut::context() const
{
    Q_D(const QShortcut);
    return d->sc_context;
}

/*!
    \property QShortcut::autoRepeat
    \brief whether the shortcut can auto repeat

    If true, the shortcut will auto repeat when the keyboard shortcut
    combination is held down, provided that keyboard auto repeat is
    enabled on the system.
    The default value is true.
*/
void QShortcut::setAutoRepeat(bool on)
{
    Q_D(QShortcut);
    if (d->sc_autorepeat == on)
        return;
    QAPP_CHECK("setAutoRepeat");
    d->sc_autorepeat = on;
    QGuiApplicationPrivate::instance()->shortcutMap.setShortcutAutoRepeat(on, d->sc_id, this);
}

bool QShortcut::autoRepeat() const
{
    Q_D(const QShortcut);
    return d->sc_autorepeat;
}


/*!
    \property QShortcut::whatsThis
    \brief the shortcut's "What's This?" help text

    The text will be shown when a widget application is in "What's
    This?" mode and the user types the shortcut key() sequence.

    To set "What's This?" help on a menu item (with or without a
    shortcut key), set the help on the item's action.

    By default, this property contains an empty string.

    This property has no effect in applications that don't use
    widgets.

    \sa QWhatsThis::inWhatsThisMode(), QAction::setWhatsThis()
*/
void QShortcut::setWhatsThis(const QString &text)
{
    Q_D(QShortcut);
    d->sc_whatsthis = text;
}

QString QShortcut::whatsThis() const
{
    Q_D(const QShortcut);
    return d->sc_whatsthis;
}


/*!
    Returns the shortcut's ID.

    \sa QShortcutEvent::shortcutId()
*/
int QShortcut::id() const
{
    Q_D(const QShortcut);
    return d->sc_id;
}

/*!
    \fn QWidget *QShortcut::parentWidget() const

    Returns the shortcut's parent widget.
*/

/*!
    \internal
*/
bool QShortcut::event(QEvent *e)
{
    Q_D(QShortcut);
    if (d->sc_enabled && e->type() == QEvent::Shortcut) {
        auto se = static_cast<QShortcutEvent *>(e);
        if (se->shortcutId() == d->sc_id && se->key() == d->sc_sequence
            && !d->handleWhatsThis()) {
            if (se->isAmbiguous())
                emit activatedAmbiguously();
            else
                emit activated();
            return true;
        }
    }
    return QObject::event(e);
}

QT_END_NAMESPACE

#include "moc_qshortcut.cpp"
