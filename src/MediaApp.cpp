#include <Wt/WApplication.h>
#include <Wt/WSelectionBox.h>
#include <Wt/WPushButton.h>
#include <Wt/WStringListModel.h>
#include <Wt/WAny.h>
#include <boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>
#include <vector>
#include <string>
#include <fstream>
#include "MediaApp.h"
#include "MediaPlayer.h"
#include <Wt/WStandardItemModel.h>
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WInPlaceEdit.h>
#include <Wt/WFileUpload.h>
#include <Wt/WProgressBar.h>
#include <Wt/WText.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WMediaPlayer.h>
#include <Wt/WLink.h>
#include <iostream>
#include <unordered_map>
#include <Wt/WRegExpValidator.h>
#include <Wt/WLineEdit.h>
#include <Wt/WLabel.h>
#include <Wt/WEnvironment.h>
#include <Wt/WDialog.h>
#include <Wt/WBreak.h>
#include <Wt/WComboBox.h>
#include <Wt/WCheckBox.h>

Wt::WComboBox* folderSelectionBox;
Wt::WSelectionBox* fileSelectionBox;
Wt::WMediaPlayer *player;
Wt::WInteractWidget *playButton;
Wt::WInteractWidget *overlayPlayButton;
Wt::WInteractWidget *stopButton;
Wt::WPushButton *mediaButton;
Wt::WPushButton *favoritesButton;
Wt::WCheckBox *showFavoritesCheckBox;
std::unordered_map<std::string, bool> favorites;
std::unordered_map<std::string, double> playbackTime;
std::string const homeDirectory = "./Media";
std::string jsCode, currentDirectory;
Wt::WText *currentSelection;

MediaApp::MediaApp(const Wt::WEnvironment& env)
  : WApplication(env)
{
    
	// Sets the title of the page tab
    setTitle("Personal Media Server");
    useStyleSheet("style.css");

	// Adds a title to the webpage
    auto title = root()->addWidget(std::make_unique<Wt::WText>("Personal Media Server"));
    title->addStyleClass("title-text");
  
    auto model = std::make_shared<Wt::WStringListModel>();	//Model used to store files to be displayed in selectionbox
	auto fmodel = std::make_shared<Wt::WStringListModel>();	//Model used to store directories for the folder selection combobox
    
	fileSelectionBox = root()->addNew<Wt::WSelectionBox>(); //The SelectionBox Widget
    populateSelectionBox(homeDirectory, model);	//Populates the selectionbox widget with contents of model
	fileSelectionBox->addStyleClass("selection-box");
	
	auto *searchBar = root()->addNew<Wt::WInPlaceEdit>("Search");	//Search bar widget for filtering files displayed in the selectionbox
	
	folderSelectionBox = root()->addNew<Wt::WComboBox>();	//ComboBox widget for navigating to different folders
	populateFolderBox(homeDirectory, fmodel);	//Populates the folderSelectionBox widget with contents of model
	folderSelectionBox->addStyleClass("combo-box");
	
	auto nFolder = root()->addWidget(std::make_unique<Wt::WPushButton>("Add Folder"));	//Button for adding folders
    auto rButton = root()->addWidget(std::make_unique<Wt::WPushButton>("Delete")); //Delete button ***WILL ACTUALLY DELETE FILE FROM SYSTEM***

	
	
    Wt::WFileUpload *fu = root()->addNew<Wt::WFileUpload>();	//Widget for selecting files to upload to server
	fu->setMultiple(false);	//Disables ability to upload multiple files at once	
	fu->setFilters(".mp3,.mp4");	//Filters file selection to display only .mp3/.mp4 files
    fu->setProgressBar(std::make_unique<Wt::WProgressBar>());	//Adds progress bar to display file upload progress
    fu->setMargin(10, Wt::Side::Right | Wt::Side::Left);
	
    Wt::WPushButton *uploadButton = root()->addNew<Wt::WPushButton>("Add");  //Button to initiate upload of selected file
    uploadButton->setMargin(10, Wt::Side::Left | Wt::Side::Right);
	uploadButton->disable();
    Wt::WText *out = root()->addNew<Wt::WText>();
   
	//Changing the default style of widget and text of buttons
	searchBar->setPlaceholderText("Search");
	searchBar->setMargin(10, Wt::Side::Right | Wt::Side::Left);
	searchBar->setWidth(100);
	searchBar->addStyleClass("search-bar");
	searchBar->saveButton()->setText("Search");
	Wt::WPushButton* sCancel = searchBar->cancelButton();
    sCancel->setText("Clear");
    
    Wt::WPushButton *returnButton = root()->addNew<Wt::WPushButton>("Home Folder");	//Button to navigate back to the Main media folder
    mediaButton = root()->addNew<Wt::WPushButton>("Play Media");	//Button to play selected file
	mediaButton->disable();
	currentSelection = root() -> addNew<Wt::WText>("");	//Text element that displays the name of the currently selected file
	currentSelection->hide();
	currentSelection->addStyleClass("selection-text");
	
    Wt::WText *notification = root()->addNew<Wt::WText>();	//Notifies user if they hit play before selecting a file
	
	
	// Add a line break
	root()->addWidget(std::make_unique<Wt::WBreak>());
	root()->addWidget(std::make_unique<Wt::WBreak>());
	
	// Initialize the favorites button
	favoritesButton = root()->addNew<Wt::WPushButton>("Add to Favorites");
	
	// Initialize the favourites checkbox
	showFavoritesCheckBox = root()->addNew<Wt::WCheckBox>("Show Favorites Only");
	showFavoritesCheckBox->addStyleClass("fav-checkbox");

	loadFavorites();

	// Connect the checkbox's changed signal
	showFavoritesCheckBox->changed().connect([=] {
		if (showFavoritesCheckBox->isChecked()) {
			populateFavourites(); // Show only favorites
		} else {
			populateSelectionBox(homeDirectory, model); // Show all files
		}
	});
    
	nFolder->clicked().connect([=] { // Return button pressed
        addFolder(homeDirectory, fmodel);
    });
	
	returnButton->clicked().connect([=] { // Return button pressed
        populateSelectionBox(homeDirectory, model);
		populateFolderBox(homeDirectory, fmodel);
		selection("");
		mediaButton->disable();
    });
	
	sCancel->clicked().connect([=] {	//Exits the search bar and clears search filters
	
		searchBar->setText("Search");
		populateSelectionBox(homeDirectory, model);
	});
	
	searchBar->valueChanged().connect([=] {
		std::string filterby = searchBar->text().toUTF8();
		populateSelectionBox(homeDirectory, model, filterby);
	});
	
	favoritesButton->clicked().connect([=] {
		std::string selectedFile = fileSelectionBox->currentText().toUTF8();
		//std::string selectedFile = fileSelectionBox->currentIndex();

		if (favorites.find(selectedFile) == favorites.end()) {
			// Add to favorites if not present
			favorites[selectedFile] = true;
			saveFavorites();
			favoritesButton->setText("Remove from Favorites");

		} else {
			// Remove from favorites if present
			favorites.erase(selectedFile);
			saveFavorites();
		
			// Refresh the selection box if 'Show Favorites Only' is checked
			if (showFavoritesCheckBox->isChecked()) {
				populateFavourites();
			} else {
				favoritesButton->setText("Add to Favorites");
			}
		}
	});
	
    mediaButton->clicked().connect([=] { // Media button pressed
	
		
		//If user presses play without selecting a file, displays a message
        if (currentSelection->text() == "") {
         
		 notification->setText("Select a file first!");
        
		
        } else {
       
            std::string fname = fileSelectionBox->currentText().toUTF8();	//Name of the file currently selected 
            int row = fileSelectionBox->currentIndex();	//Current index of the selection in the selectionbox
            std::string fpath = getFilePath(model, row);	//Gets the path of the currently selected file
            std::string media = fpath.substr(1);	
            
            //Creates an audioplayer widget if the file ends in mp3
            if (fpath.substr(fpath.length() - 3) == "mp3") {
                player = root()->addNew<Wt::WMediaPlayer>(Wt::MediaType::Audio);
                player->clearSources();
                player->addSource(Wt::MediaEncoding::MP3, Wt::WLink(media));
				overlayPlayButton = player->button(Wt::MediaPlayerButtonId::Play);
							
			
			//Creates a videoplayer widget if the file ends in mp4
            } else if (fpath.substr(fpath.length() - 3) == "mp4") {
                player = root()->addNew<Wt::WMediaPlayer>(Wt::MediaType::Video);
                player->clearSources();
                player->addSource(Wt::MediaEncoding::M4V, Wt::WLink(media));
				overlayPlayButton = player->button(Wt::MediaPlayerButtonId::VideoPlay);
				
            
			}
			
			//Variables for the play and stop buttons of the mediaplayer 
			playButton = player->button(Wt::MediaPlayerButtonId::Play);		
			stopButton = player->button(Wt::MediaPlayerButtonId::Stop);
			
			Wt::WString selectionString = currentSelection->text();
            player->setTitle(selectionString);
            player->setMargin(10);
						

			mediaButton->setDisabled(true);
            Wt::WPushButton *xButton = root()->addNew<Wt::WPushButton>("Close Player");  // Close button
            xButton->setMargin(10);
			xButton->setObjectName("closeButton");
			Wt::WApplication::instance()->doJavaScript(jsCode);
			
            xButton->clicked().connect([=] { // Media button pressed
				std::string current = fileSelectionBox->currentText().toUTF8();
				
				playbackTime[current] = player->currentTime();
				setSeekTime(playbackTime, "playback_progress.txt");
                mediaButton->setDisabled(false);
                root()->removeWidget(player);
                root()->removeWidget(xButton);
				Wt::WApplication::instance()->doJavaScript(jsCode);
            });
			
			
			playButton->clicked().connect([=] {
				playbackTime = getSeekTime("playback_progress.txt");
				player->seek(playbackTime[fname]);
			});
			
			overlayPlayButton->clicked().connect([=] {
				playbackTime = getSeekTime("playback_progress.txt");
				player->seek(playbackTime[fname]);
			});
			
			stopButton->mouseWentDown().connect([=] {
				playbackTime[fname] = player->currentTime();
				setSeekTime(playbackTime, "playback_progress.txt");
				
			});
        }
    });
	
	
	folderSelectionBox->activated().connect([=] {
		std::string fname = folderSelectionBox->currentText().toUTF8();
        int row = folderSelectionBox->currentIndex();
        std::string fpath = Wt::asString(fmodel->data(model->index(row,0), Wt::ItemDataRole::User)).toUTF8();
		
		populateSelectionBox(fpath, model);
		selection("");
		mediaButton->disable();
	});
	
    fileSelectionBox->activated().connect([=] { //Whenever an item is selected the name and path is stored in fname and fpath

        std::string fname = fileSelectionBox->currentText().toUTF8();
		int row = fileSelectionBox->currentIndex();
        std::string fpath = getFilePath(model, row);
		
		// Update favorites button text
		if (favorites.find(fname) != favorites.end()) {
			favoritesButton->setText("Remove from Favorites");
		} else {
			favoritesButton->setText("Add to Favorites");
		}
		
        
        selection("    Current Selection: "+ fname + "     ");
		mediaButton->enable();	
		
		if (boost::filesystem::is_directory(fpath)){
			mediaButton->setText("Open Folder");
		} else {
			mediaButton->setText("Play Media");
		}
        notification->setText("");
		
    });

    rButton->clicked().connect([=] {    //Delete button pressed
        int row = fileSelectionBox->currentIndex();
        std::string fpath = getFilePath(model, row);
		
        model->removeRows(row, 1);  //Removes entry from selectionbox
        selection("");
		
        if (boost::filesystem::exists(fpath)){
            
            boost::filesystem::remove_all(fpath);     //DELETES FILE FROM DIRECTORY
        
        } else {
            std::cout << "File not Found.";
        }
    });

    uploadButton->clicked().connect([=] {         //add button pressed
        fu->upload();
		uploadButton->disable();
    });
	
	fu->changed().connect([=] {
		std::string namefile;
		namefile = fu->toolTip().toUTF8();
		uploadButton->enable();
	});
   
	//Reacts to a succesfull upload
    fu->uploaded().connect([=] {
       std::string mFilename = fu->spoolFileName();
	   std::string fname = mFilename.substr(5);
	   std::string trueFilename = fu->clientFileName().toUTF8();
	   std::string dest = currentDirectory + trueFilename;
       out->setText("File upload is finished.");
       out->setText(mFilename);
	   fu->stealSpooledFile();
	   
	   boost::filesystem::path sourcePath(mFilename);
	   boost::filesystem::path destinationDirectory(dest);
	   boost::filesystem::copy_file(sourcePath, destinationDirectory);
	   this->doJavaScript("location.reload();");
});
    
	//Reacts to a file upload problem
    fu->fileTooLarge().connect([=] {
       out->setText("File is too large.");
});       
	
	jsCode = R"(
	function createOverlay() {
		var overlay = document.createElement('div');
		overlay.setAttribute('id', 'dim-overlay');
		overlay.style.position = 'fixed';
		overlay.style.width = '100%';
		overlay.style.height = '100%';
		overlay.style.top = '0';
		overlay.style.left = '0';
		overlay.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
		overlay.style.zIndex = '10';
		document.body.appendChild(overlay);
	}
	
	function removeOverlay() {
		var overlay = document.getElementById('dim-overlay');
		if (overlay) {
			overlay.parentNode.removeChild(overlay);
		}
	}
	
	// Run createOverlay if your element is on the page
	if (document.querySelector('.jp-video') || document.querySelector('.jp-audio')) {
		createOverlay();
	} else {
		removeOverlay()
	}
	)";

}

void MediaApp::saveFavorites() {
    std::ofstream out("favorites.txt");
	
    for (const auto& item : favorites) {
        if (item.second) { // If it's a favorite
            out << item.first << std::endl;
        }
    }
	
    out.close();
}

void MediaApp::loadFavorites() {
    std::ifstream in("favorites.txt");
    std::string line;
	
    while (std::getline(in, line)) {
        favorites[line] = true;
    }
	
    in.close();
}

void MediaApp::populateFavourites() {
    auto favouritesModel = std::make_shared<Wt::WStringListModel>();
	
    for (const auto& item : favorites) {
        if (item.second) { // Check if the item is marked as a favorite
            favouritesModel->addString(item.first); // Add only favorites to the model
        }
    }
	
    fileSelectionBox->setModel(favouritesModel);
}

void MediaApp::populateSelectionBox(const std::string& directoryPath, std::shared_ptr<Wt::WStringListModel> model, const std::string& filter) { //Adds elements to SelctionBox
    namespace fs = boost::filesystem;
	std::vector<std::string> directory_fVector, directory_Vector, idata;
    std::vector<Wt::WString> emptyStringList;
    
    try {
        fs::path directory(directoryPath);
        
        if (fs::is_directory(directory)) {
        
			model->setStringList(emptyStringList);
			
            for (const fs::directory_entry& entry : fs::directory_iterator(directory)) { //Iterates through each file in a directory
                         
                std::string fileName = entry.path().filename().string();    //Generates filename string
                std::string filePath = directoryPath + "/" + fileName;    //Holds the path to the selected file
				
                if (fileName.substr(fileName.length() - 4) == ".mp3" || fileName.substr(fileName.length() - 4) == ".mp4"){
					
					directory_Vector.push_back(fileName + "|" + filePath);
				}
            }
			
            std::sort(directory_Vector.begin(), directory_Vector.end());			
			
			
			for (const std::string& item : directory_Vector){
				
				boost::split(idata, item, boost::is_any_of("|"));
				std::string fname = idata[0];
				
				if (fname.find(filter) != std::string::npos){
					
					model->addString(idata[0]);
					model->setData(model->rowCount() - 1, 0, std::string(idata[1]), Wt::ItemDataRole::User);
				}
			}
            fileSelectionBox->setModel(model);
			currentDirectory = directoryPath + "/"; //Stores current directory of selectionbox
            
        }
        
    } catch (const fs::filesystem_error& ex) {
        std::cout << "Cannot access directory";
    }
}

void MediaApp::populateFolderBox(const std::string& directoryPath, std::shared_ptr<Wt::WStringListModel> fmodel) {
	namespace fs = boost::filesystem;
	std::vector<std::string> directory_fVector, directory_Vector, idata;
    std::vector<Wt::WString> emptyStringList;
	
	try {
        fs::path directory(directoryPath);
        
        if (fs::is_directory(directory)) {
        
			fmodel->setStringList(emptyStringList);
			
            for (const fs::directory_entry& entry : fs::directory_iterator(directory)) { //Iterates through each file in a directory
                    
				if (fs::is_directory(entry)){		
				
					std::string directoryName = entry.path().filename().string();
					std::string directoryPath = entry.path().string();    //Generates filename string
					
					directory_Vector.push_back(directoryName + "|" + directoryPath);
								
				}			           
            }
			
            std::sort(directory_Vector.begin(), directory_Vector.end());
				
			
			for (const std::string& item : directory_Vector){
				
				boost::split(idata, item, boost::is_any_of("|"));
				std::string fname = idata[0];
							
					fmodel->addString(idata[0]);
					fmodel->setData(fmodel->rowCount() - 1, 0, std::string(idata[1]), Wt::ItemDataRole::User);			
			}
			
			folderSelectionBox->setNoSelectionEnabled(true);
            folderSelectionBox->setModel(fmodel);	
            
        } else {
            Wt::log("error") << "The specified path is not a directory.";
        }
        
    } catch (const fs::filesystem_error& ex) {
        std::cout << "Error accessing the directory";
    }
}

std::string MediaApp::getFilePath(std::shared_ptr<Wt::WStringListModel> m, int& i){
	
	std::string path, fdata;
	std::vector<std::string> pdata;
	
	fdata = Wt::asString(m->data(m->index(i,0), Wt::ItemDataRole::User)).toUTF8();
	
	boost::split(pdata, fdata, boost::is_any_of("|"));
	
	path = pdata[0];
	
	
	return path;
}

std::unordered_map<std::string, double> MediaApp::getSeekTime(const std::string& progFile){
	
	double t;
	std::unordered_map<std::string, double> fTimes;
	std::ifstream file(progFile);
	std::string line, name;
	
	while(std::getline(file, line)){
		
		size_t pos = line.find(":");
		
		if (pos != std::string::npos) {
			
			std::string nFile = line.substr(0, pos);
			t = std::stod(line.substr(pos + 1));
			fTimes[nFile] = t;
		}
	}
	
	return fTimes;
}

void MediaApp::setSeekTime(std::unordered_map<std::string, double> fTimes, const std::string& progFile){
	
	std::string fdata;
	std::vector<std::string> pdata;
	
	std::ofstream file(progFile);
	
	for (const auto& entry : fTimes){
		file << entry.first << ":" << entry.second << "\n";
	}
}

void MediaApp::addFolder(const std::string& directoryPath, std::shared_ptr<Wt::WStringListModel> fmodel) {
	
	auto *dialog = root()->addNew<Wt::WDialog>("Create a New Folder");
	Wt::WLabel *label = dialog->contents()->addNew<Wt::WLabel>("Enter Folder Name:");
	Wt::WLineEdit *name = dialog->contents()->addNew<Wt::WLineEdit>();
	label->setBuddy(name);
	
	
	dialog->contents()->addStyleClass("dialog.css");
	auto validator = std::make_shared<Wt::WRegExpValidator>("^(?!.*(?:\\.mp3|\\.mp4)$)[^/\\:|<>?*\"]{2,20}$");
    validator->setMandatory(true);
    name->setValidator(validator);
	
	Wt::WPushButton *add = dialog->footer()->addNew<Wt::WPushButton>("Add");
    add->setDefault(true);

	Wt::WPushButton *cancel = dialog->footer()->addNew<Wt::WPushButton>("Cancel");
    dialog->rejectWhenEscapePressed();
	
	name->keyWentUp().connect([=] {
        add->setDisabled(name->validate() != Wt::ValidationState::Valid);
    });
	
	add->clicked().connect([=] {
        if (name->validate() == Wt::ValidationState::Valid)
            dialog->accept();
    });
	
	dialog->finished().connect([=] {
		std::string nDirectory, dName;
		dName = name->text().toUTF8();
        nDirectory = homeDirectory + "/" + dName;
		boost::filesystem::create_directory(nDirectory);
		populateFolderBox(homeDirectory, fmodel);
        root()->removeWidget(dialog);
    });
	
	cancel->clicked().connect(dialog, &Wt::WDialog::reject);
	dialog->show();
	
}

void MediaApp::selection(std::string curSelection){
	
	if (curSelection == ""){
		
		currentSelection->hide();
		
	} else {
		
		currentSelection->setText(curSelection);
		currentSelection->show();
	}
}



