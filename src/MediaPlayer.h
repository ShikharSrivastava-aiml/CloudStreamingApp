#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <Wt/WApplication.h>
#include <Wt/WLink.h>
#include <Wt/WMediaPlayer.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WFileUpload.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>


class MediaPlayer : public Wt::WApplication {
public:
    MediaPlayer(const Wt::WEnvironment& env, char mediaType);
    void addSource(std::string file, Wt::MediaEncoding mediaType);
private:
    Wt::WMediaPlayer* mediaPlayer;

};

#endif // MEDIAPLAYER_H