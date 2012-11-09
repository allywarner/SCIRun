/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <stdexcept>
#include <QtGui>
#include <Interface/Application/Connection.h>
#include <Interface/Application/Utility.h>
#include <Interface/Application/Port.h>
#include <Interface/Application/GuiLogger.h>

using namespace SCIRun::Gui;

ConnectionLine::ConnectionLine(PortWidget* fromPort, PortWidget* toPort, const SCIRun::Dataflow::Networks::ConnectionId& id)
  : fromPort_(fromPort), toPort_(toPort), id_(id), destroyed_(false)
{
  if (fromPort_)
  {
    fromPort_->addConnection(this);
    fromPort_->turn_on_light();
  }
  if (toPort_)
  {
    toPort_->addConnection(this);
    toPort_->turn_on_light();
  }

  setFlags(QGraphicsItem::ItemIsSelectable);
  //TODO: need dynamic zValue
  setZValue(100);

  if (fromPort_ && toPort_)
    setColor(fromPort_->color());
  
  trackNodes();
  GuiLogger::Instance().log("Connection made.");
}

ConnectionLine::~ConnectionLine()
{
  if (!destroyed_)
    destroy();
}

void ConnectionLine::destroy() 
{
  if (fromPort_ && toPort_)
  {
    fromPort_->removeConnection(this);
    fromPort_->turn_off_light();
    toPort_->removeConnection(this);
    toPort_->turn_off_light();
  }
  Q_EMIT deleted(id_);
  GuiLogger::Instance().log("Connection deleted.");
  destroyed_ = true;
}

void ConnectionLine::setColor(const QColor& color)
{
  setPen(QPen(color, 5.0));
}

QColor ConnectionLine::color() const
{
  return pen().color();
}

void ConnectionLine::trackNodes()
{
  if (fromPort_ && toPort_)
  {
    setLine(QLineF(fromPort_->position(), toPort_->position()));
  }
  else
    BOOST_THROW_EXCEPTION(InvalidConnection() << Core::ErrorMessage("no from/to set for Connection"));
}

void ConnectionLine::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
  const char* del = "Delete";
  QMenu menu;
  menu.addAction(del);
  auto a = menu.exec(event->screenPos());
  if (a && a->text() == del)
  {
    scene()->removeItem(this);
    destroy();
  }
}

ConnectionInProgress::ConnectionInProgress(PortWidget* port)
  : fromPort_(port)
{
  setZValue(1000);

  setColor(port->color());
}

void ConnectionInProgress::setColor(const QColor& color)
{
  setPen(QPen(color, 5.0, Qt::DashLine));
}

QColor ConnectionInProgress::color() const
{
  return pen().color();
}

void ConnectionInProgress::update(const QPointF& end)
{
  setLine(QLineF(fromPort_->position(), end));
}

