<?php

namespace xmlSitemapGenerator;

// settings for generating a map



class sitemapDefaults {
	
	function __construct() {
		//($exclude1 = 1,$priority1 = 1,$frequency1 = 1, $inherit = 0)
		$this->homepage = new metaSettings(3,12,6,0);	
		$this->pages = new metaSettings(3,8,4,0);	
		$this->posts = new metaSettings(3,8,4,0);	
		$this->taxonomyCategories = new metaSettings(3,5,5,0);	
		$this->taxonomyTags = new metaSettings(3,5,5,0);	
	 
		$this->recentArchive = new metaSettings(3,8,7,0);	
		$this->oldArchive = new metaSettings(3,5,3,0);	
		$this->authors = new metaSettings(3,5,5,0);	
		 
	}	
	
	public $homepage ;
	public $pages;
	public $posts ;
	public $taxonomyCategories;
	public $taxonomyTags;
 
	public $recentArchive ;
	public $oldArchive ;
	public $authors ;

	public $dateField = "updated"; // date field for sitemap can be updated or created date.
	public $excludeRules = "";

	public $customPosts = array();
}

class globalSettings {
	
	public $addRssToHead = true;  // add recent files to Rss header
	public $pingSitemap = true; // daily sitemap ping
	public $addToRobots = true; // add files to robots
	public $sendStats = true; // send stats
	public $smallCredit = true; //allow a credit in the sitemap footer
	public $registerEmail = "";
	public $register = false;
	
	public $urlXmlSitemap = "xmlsitemap.xml";
	public $urlRssSitemap = "rsssitemap.xml";
	public $urlRssLatest = "rsslatest.xml";
	public $urlHtmlSitemap = "htmlsitemap.htm";
}

class metaSettings
{
	function __construct($exclude1 = 1,$priority1 = 1,$frequency1 = 1, $inherit = 0) {
		$this->exclude = $exclude1;
		$this->priority = $priority1;
		$this->frequency = $frequency1;
	}
 
	public $id = 0;
	public $itemId = 0;
	public $itemType = "";
	public $exclude = 1;
	public $priority = 1;		
	public $frequency = 1;
	public $inherit = 0;
	public $scheduled = 0;

}	 


?>