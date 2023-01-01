/*
 *  Copyright (c) 2021-2021 shangu. All Right Reserved
 *
 *  author:  shangu
 *  Date:    2021-mm-dd
 *
 *  Description: 
 *
*/

#pragma once

#include <cassert>
#include <functional>

#include <QApplication>
#include <QThreadPool>
#include <QPointer>
#include <QEvent>

namespace valley {
    namespace qt {

        template<typename Base>
        class Application : public Base
        {
            struct CompletedEvent : public QEvent
            {
                using token_t = std::function<void()>;

                QPointer<QObject> reciever;
                token_t token;

                template<typename F>
                CompletedEvent(int type, QObject* reciever, F&& f) : QEvent(QEvent::Type(type)),
                    reciever(reciever),
                    token(std::move(f))
                {}
            };

        public:
            Application(int argc, char* argv[]) : Base(argc, argv),
                completed_event_type(QEvent::registerEventType()),
                thread_pool(this)
            {}

            template<typename F>
            static void async(F&& f)
            {
                auto* app = dynamic_cast<Application*>(QApplication::instance());
                app->thread_pool.tryStart(std::move(f));
            }

            template<typename F, typename CompletedToken>
            static void async_then(QObject* reciever, F&& f, CompletedToken&& token)
            {
                auto* app = dynamic_cast<Application*>(QApplication::instance());
                QEvent* e = new CompletedEvent(app->completed_event_type,
                    reciever,
                    std::move(token));

                app->thread_pool.tryStart(
                    [app, f, e]() {
                        f();
                        app->postEvent(app, e);
                    });
            }

        protected:
            bool event(QEvent* e) override {

                if (e->type() == completed_event_type)
                {
                    auto* ce = dynamic_cast<CompletedEvent*>(e);
                    if (ce->reciever)
                        ce->token();

                    return true;
                }
                else {
                    return Base::event(e);
                }
            }

        private:
            const int completed_event_type;
            QThreadPool thread_pool;
        };
    }
}

using Application = valley::qt::Application<QApplication>;