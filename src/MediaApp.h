#include <Wt/WApplication.h>
#include <Wt/WSelectionBox.h>
#include <unordered_map>

class MediaApp : public Wt::WApplication
{
public:

  MediaApp(const Wt::WEnvironment& env);

private:
	//The selectionbox which displays the files in the current folder	
	Wt::WSelectionBox* fileSelectionBox;
	
	//Saves which files are marked as favorited	to favorites.txt file
	void saveFavorites();

	//Loads which files are marked as favorited from favorites.txt file
	void loadFavorites();

	//Updates the favorited state of the files
	void populateFavourites();
	    
	void populateSelectionBox(const std::string& directoryPath, std::shared_ptr<Wt::WStringListModel> model, const std::string& filter="");
	  
	std::string getFilePath(std::shared_ptr<Wt::WStringListModel> m, int& i);
	  
	std::unordered_map<std::string, double> getSeekTime(const std::string& progFile);
	  
	void setSeekTime(std::unordered_map<std::string, double> fTimes, const std::string& progFile);
	  
	void addFolder(const std::string& directoryPath, std::shared_ptr<Wt::WStringListModel> model);
	  
	void populateFolderBox(const std::string& directoryPath, std::shared_ptr<Wt::WStringListModel> fmodel);
	  
	void selection(std::string curSelection);
  
};

