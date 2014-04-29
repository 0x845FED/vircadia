//
//  FramelessDialog.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "FramelessDialog.h"

const int RESIZE_HANDLE_WIDTH = 7;

FramelessDialog::FramelessDialog(QWidget *parent, Qt::WindowFlags flags, Position position) :
        QDialog(parent, flags | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
        _isResizing(false),
        _resizeInitialWidth(0),
        _selfHidden(false),
        _position(position),
        _hideOnBlur(true) {

    setAttribute(Qt::WA_DeleteOnClose);

    // handle rezize and move events
    parentWidget()->installEventFilter(this);

    // handle minimize, restore and focus events
    Application::getInstance()->installEventFilter(this);
}

bool FramelessDialog::eventFilter(QObject* sender, QEvent* event) {
    switch (event->type()) {
        case QEvent::Move:
            if (sender == parentWidget()) {
                resizeAndPosition(false);
            }
            break;
        case QEvent::Resize:
            if (sender == parentWidget()) {
                resizeAndPosition(false);
            }
            break;
        case QEvent::WindowStateChange:
            if (_hideOnBlur && parentWidget()->isMinimized()) {
                if (isVisible()) {
                    _selfHidden = true;
                    setHidden(true);
                }
            } else if (_selfHidden) {
                _selfHidden = false;
                setHidden(false);
            }
            break;
        case QEvent::ApplicationDeactivate:
            // hide on minimize and focus lost
            if (_hideOnBlur && isVisible()) {
                _selfHidden = true;
                setHidden(true);
            }
            break;
        case QEvent::ApplicationActivate:
            if (_selfHidden) {
                _selfHidden = false;
                setHidden(false);
            }
            break;
        default:
            break;
    }

    return false;
}

void FramelessDialog::setStyleSheetFile(const QString& fileName) {
    QFile globalStyleSheet(Application::resourcesPath() + "styles/global.qss");
    QFile styleSheet(Application::resourcesPath() + fileName);
    if (styleSheet.open(QIODevice::ReadOnly) && globalStyleSheet.open(QIODevice::ReadOnly) ) {
        QDir::setCurrent(Application::resourcesPath());
        setStyleSheet(globalStyleSheet.readAll() + styleSheet.readAll());
    }
}

void FramelessDialog::showEvent(QShowEvent* event) {
    resizeAndPosition();
    QDialog::showEvent(event);
}

void FramelessDialog::resizeAndPosition(bool resizeParent) {
    // keep full app height or width depending on position
    if (_position == POSITION_LEFT || _position == POSITION_RIGHT) {
        setFixedHeight(parentWidget()->size().height());
    } else {
        setFixedWidth(parentWidget()->size().width());
    }

    // resize parrent if width is smaller than this dialog
    if (resizeParent && parentWidget()->size().width() < size().width()) {
        parentWidget()->resize(size().width(), parentWidget()->size().height());
    }

    if (_position == POSITION_LEFT || _position == POSITION_TOP) {
        // move to upper left corner
        move(parentWidget()->geometry().topLeft());
    } else if (_position == POSITION_RIGHT) {
        // move to upper right corner
        QPoint pos = parentWidget()->geometry().topRight();
        pos.setX(pos.x() - size().width());
        move(pos);
    }
    repaint();
}

void FramelessDialog::mousePressEvent(QMouseEvent* mouseEvent) {
    if (mouseEvent->button() == Qt::LeftButton) {
        if (_position == POSITION_LEFT || _position == POSITION_RIGHT) {
            bool hitLeft = (_position == POSITION_LEFT) && (abs(mouseEvent->pos().x() - size().width()) < RESIZE_HANDLE_WIDTH);
            bool hitRight = (_position == POSITION_RIGHT) && (mouseEvent->pos().x() < RESIZE_HANDLE_WIDTH);
            if (hitLeft || hitRight) {
                _isResizing = true;
                _resizeInitialWidth = size().width();
                QApplication::setOverrideCursor(Qt::SizeHorCursor);
            }
        } else {
            bool hitTop = (_position == POSITION_TOP) && (abs(mouseEvent->pos().y() - size().height()) < RESIZE_HANDLE_WIDTH);
            if (hitTop) {
                _isResizing = true;
                _resizeInitialWidth = size().height();
                QApplication::setOverrideCursor(Qt::SizeHorCursor);
            }
        }
    }
}

void FramelessDialog::mouseReleaseEvent(QMouseEvent* mouseEvent) {
    QApplication::restoreOverrideCursor();
    _isResizing = false;
}

void FramelessDialog::mouseMoveEvent(QMouseEvent* mouseEvent) {
    if (_isResizing) {
        if (_position == POSITION_LEFT) {
            resize(mouseEvent->pos().x(), size().height());
        } else if (_position == POSITION_RIGHT) {
            setUpdatesEnabled(false);
            resize(_resizeInitialWidth - mouseEvent->pos().x(), size().height());
            resizeAndPosition();
            _resizeInitialWidth = size().width();
            setUpdatesEnabled(true);
        } else if (_position == POSITION_TOP) {
            resize(size().width(), mouseEvent->pos().y());
        }
    }
}
