#include <cstdio>

#include <termios.h>
#include <unistd.h>

#include <gst/gst.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

#if __has_include(<experimental/scope>) && __cplusplus >= 202002L
#include <experimental/scope>
using std::experimental::scope_exit;
#else
template <typename F>
class scope_exit {
public:
    ~scope_exit() noexcept {
        exit_fun();
    }

    template <typename Fp>
    explicit
    scope_exit(Fp &&f) noexcept
      : exit_fun(std::forward<Fp>(f))
    {}

private:
    F exit_fun;
};
template <typename F>
scope_exit(F) -> scope_exit<F>;
#endif

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    fmt::print("USAGE: Choose one of the following options:\n"
        " 'P' to toggle between PAUSE and PLAY\n"
        " 'S' to increase playback speed, 's' to decrease playback speed\n"
        " 'D' to toggle playback direction\n"
        " 'N' to move to next frame (in the current direction, better in PAUSE)\n"
        " 'Q' to quit\n");

    // Changing the playback rate probably won't work with an HTTPS source.
    // Please download and store the file locally to use all functionality.
    auto pipeline = Gst::parse_launch(
        "playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm");

    auto video_sink = Gst::Element{};
    auto playing    = true;
    auto rate       = 1.0;
    auto loop       = GLib::MainLoop::new_(nullptr, false);

    // Send seek event to change rate
    auto send_seek_event = [&]() {
        auto position = gint64{};

        // Obtain the current position, needed for the seek event
        if (!pipeline.query_position(Gst::Format::TIME_, &position)) {
            fmt::print(stderr, "Unable to retrieve current position.\n");
            return;
        }

        auto seek_event = Gst::Event{};

        // Create the seek event
        if (0 < rate) {
            seek_event = Gst::Event::new_seek(rate, Gst::Format::TIME_
              , Gst::SeekFlags::FLUSH_ | Gst::SeekFlags::ACCURATE_
              , Gst::SeekType::SET_
              , position
              , Gst::SeekType::END_, 0);
        } else {
            seek_event = Gst::Event::new_seek(rate, Gst::Format::TIME_
              , Gst::SeekFlags::FLUSH_ | Gst::SeekFlags::ACCURATE_
              , Gst::SeekType::SET_, 0
              , Gst::SeekType::SET_, position);
        }

        if (!video_sink) {
            // If we have not done so, obtain the sink through which we will
            // send the seek events
            video_sink = pipeline.get_property("video-sink").get_value<Gst::Element>();
        }

        // Send the event
        video_sink.send_event(std::move(seek_event));

        fmt::print("Current rate: {}\n", rate);
    };

    auto io_stdin = GLib::IOChannel::unix_new(fileno(stdin));
    GLib::io_add_watch(io_stdin, GLib::PRIORITY_DEFAULT_, GLib::IOCondition::IN_, [&](GLib::IOChannel_Ref source, GLib::IOCondition condition) {
        auto [status, ch] = io_stdin.read_unichar();
        if (GLib::IOStatus::NORMAL_ != status) {
            return true;
        }
        switch (GLib::ascii_tolower(ch)) {
            case gunichar('p'): {
                playing = !playing;
                pipeline.set_state(playing ? Gst::State::PLAYING_ : Gst::State::PAUSED_);
                fmt::print("Setting state to {}\n", playing ? "PLAYING" : "PAUSE");
                break;
            }
            case gunichar('s'): {
                if (GLib::unichar_isupper(ch)) {
                    rate *= 2.0;
                } else {
                    rate /= 2.0;
                }
                send_seek_event();
                break;
            }
            case gunichar('d'): {
                rate *= -1.0;
                send_seek_event();
                break;
            }
            case gunichar('n'): {
                if (!video_sink) {
                    // If we have not done so, obtain the sink through which we
                    // will send the step events
                    video_sink = pipeline.get_property("video-sink").get_value<Gst::Element>();
                }
                video_sink.send_event(Gst::Event::new_step(
                    Gst::Format::BUFFERS_
                  , 1
                  , std::abs(rate)
                  , true
                  , false));
                fmt::print("Stepping one frame\n");
                break;
            }
            case gunichar('q'): {
                loop.quit();
                break;
            }
            default: {
                break;
            }
        }
        return true;
    });

    // Start playing
    auto ret = pipeline.set_state(Gst::State::PLAYING_);
    if (Gst::StateChangeReturn::FAILURE_ == ret) {
        fmt::print(stderr, "Unable to set the pipeline to the playing state.\n");
        return -1;
    }

    // Put the terminal into a raw mode, so we can read individual characters
    // without requiring pressing Enter.
    auto oldt = termios{};
    auto newt = termios{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    auto guard = scope_exit([&] {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    });

    loop.run();

    pipeline.set_state(Gst::State::NULL_);
}
