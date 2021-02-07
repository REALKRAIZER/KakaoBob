<?php

namespace xmlSitemapGenerator;

include_once 'dataAccess.php';
include_once 'renderers.php';

	class mapItem
	{
	 
		function __construct() 
		{}
		
		public $location ;
		public $title ;
		public $description ;
		public $modified  ;
		public $priority ;
		public $frequency ;
		
	}

	class sitemapBuilder 
	{

	
		private $pageSize ;
		private $urlsList = array();
		private $isExecuting = false;
		private $siteName = "";
		
		private $instance;

		public function __construct() { 
		
			$this->pageSize = get_option('posts_per_page');
			$this->urlsList = array();	
			$siteName = get_option('blogname');
		
		}
/*
		public static function getInstance();
		{
				if ($this->instance === null) {
					$this->instance = new self();
				}
				return $this->instance;
		}
	*/	
		function getValue($value, $default)
		{
			if (isset($value))
			{
				if ($value == 'default') return $default;
				return $value;
			}
			else
			{
				return $default;
			}
		}
		
		
	 
 
		function isIncluded($url, $rules)
		{
			//todo
			
			return true;
			
		}

		public static function renderComment()
		{
			echo "<!-- Created using XmlSitemapGenerator.org WordPress Plugin - Free HTML, RSS and XML sitemap generator -->";
		}
		
		
	function getPageLink($url, $pagenum = 1, $escape = true ) {
		global $wp_rewrite;
	 
		$pagenum = (int) $pagenum;
	 
		$home_root = preg_quote( home_url(), '|' );
		$request = $url;

		
		$request = preg_replace('|^'. $home_root . '|i', '', $request);
		$request = preg_replace('|^/+|', '', $request);
	 
		
		if ( !$wp_rewrite->using_permalinks() || is_admin() ) {
			$base = trailingslashit( get_bloginfo( 'url' ) );
		
			if ( $pagenum > 1 ) {
				$result = add_query_arg( 'paged', $pagenum, $base . $request );
			} else {
				$result = $base . $request;
			}
		} else {
			$qs_regex = '|\?.*?$|';
			preg_match( $qs_regex, $request, $qs_match );
	 
			if ( !empty( $qs_match[0] ) ) {
				$query_string = $qs_match[0];
				$request = preg_replace( $qs_regex, '', $request );
			} else {
				$query_string = '';
			}
	 
			$request = preg_replace( "|$wp_rewrite->pagination_base/\d+/?$|", '', $request);
			$request = preg_replace( '|^' . preg_quote( $wp_rewrite->index, '|' ) . '|i', '', $request);
			$request = ltrim($request, '/');
	 
			$base = trailingslashit( get_bloginfo( 'url' ) );
	 
			if ( $wp_rewrite->using_index_permalinks() && ( $pagenum > 1 || '' != $request ) )
				$base .= $wp_rewrite->index . '/';
	 
			if ( $pagenum > 1 ) {
				$request = ( ( !empty( $request ) ) ? trailingslashit( $request ) : $request ) . user_trailingslashit( $wp_rewrite->pagination_base . "/" . $pagenum, 'paged' );
			}
	 
			$result = $base . $request . $query_string;
		}
	 
		/**
		 * Filter the page number link for the current request.
		 *
		 * @since 2.5.0
		 *
		 * @param string $result The page number link.
		 */
		$result = apply_filters( 'get_pagenum_link', $result );
	 
		if ( $escape )
			return esc_url( $result );
		else
			return esc_url_raw( $result );
}
		
		function isExcluded($value)
		{
			if (isset($value)) 
			{
				if (value==2) {return true;}
			}
			return false;
		}
		
		function getMetaValue($postValue,$tagValue,$default)
		{
			
			if (isset($postValue)) 
			{
				
				if ( $postValue != 1) { return $postValue; }
			}
			
			if (isset($tagValue)) 
			{
				if ( $tagValue != 1) {return $tagValue; }
			}
		
			return $default;
		
			
		}
		
	
		function addUrls($postCount, $mapItem)
		{
			$pages = 1;
	
			if ($postCount > $this->pageSize)
			{
				$pages =  ceil($postCount / $this->pageSize);
			}
			
			$siteName = get_option('blogname');
			$mapItem->title = $mapItem->title . " | " . $siteName ;
			array_push($this->urlsList, $mapItem); // first page
			for ($x = 2; $x <= $pages; $x++) 
			{
				$new = clone $mapItem;
				$new->title = $new->title .  " | Page " . $x;
				$new->location = $this->getPageLink($mapItem->location,$x);	
				array_push($this->urlsList, $new);
			} 

		}	
	

	static function postTypeDefault($sitemapDefaults,$name)
	{
				if ($name == 'page')
				{
						return $sitemapDefaults->pages;
				}
				elseif ($name == 'post')
				{
						return $sitemapDefaults->posts;
				}
				else
				{
						return ( isset( $sitemapDefaults->{$name} ) ?  $sitemapDefaults->{$name} : $sitemapDefaults->posts );
				}		
	}
	
		function getPosts( $sitemapDefaults, $limit = 0){
			
		 
			$results = dataAccess::getPages( $sitemapDefaults->dateField  , $limit);
			$temp = "";
			if ($results ) {

				foreach( $results as $result ) {
					 
					
				 //	wp_cache_add($result ->ID, $result , 'posts');
					$defaults = self::postTypeDefault($sitemapDefaults,$result->post_type );
	
					$exlcude = $this->getMetaValue($result->exclude, $result->tagExclude, $defaults->exclude) ;
					
					if ($exlcude == 2) {$temp = $temp . " - excluded";  continue;}
					if ( $result->post_status =='future' && $defaults->scheduled == 0)  { continue;}
					
					$pageUrl =  get_permalink($result);	
 
					if (!($this->isIncluded($pageUrl,$sitemapDefaults->excludeRules ))) {continue;}
							
					$url = new mapItem();
					$url->location = $pageUrl  ;				
					$url->title = get_the_title( $result ); //$result->post_title;
					$url->description = $result->post_excerpt;
					$url->modified  =  $result->sitemapDate ;	
					$url->priority =    $this->getMetaValue($result->priority,$result->tagPriority,  $defaults->priority)  ;	
					$url->frequency  =  $this->getMetaValue($result->frequency,$result->tagFrequency,$defaults->frequency) ;				
				 
					$this->addUrls(0, $url);

				}
			}
			
		}
		
		function getTaxonomy($sitemapDefaults)
		{
			
			$results = dataAccess::getTaxonomy( $sitemapDefaults->dateField  );
			
			 
				
			$urls = array();
			
			if ($results ) {

				foreach( $results as $result ) {
					
					
				 //	wp_cache_add($result ->ID, $result , 'posts');
							if ($result->taxonomy == 'category')
							{$defaults = $sitemapDefaults->taxonomyCategories;}
							else
							{$defaults = $sitemapDefaults->taxonomyTags;}	
			
					$exlcude = $this->getMetaValue(null, $result->exclude, $defaults->exclude) ;
					
					if ($exlcude != 2)
					{
						$pageUrl =   get_category_link($result);	

						if ($this->isIncluded($pageUrl,$sitemapDefaults->excludeRules ))
						{
					
							$url = new mapItem();
							$url->location = $pageUrl;			
							$url->title = $result->name;
							$url->description = $result->description;
							$url->modified  =  $result->sitemapDate ;
							$url->priority = $this->getMetaValue(null,$result->priority,$defaults->priority)  ;	
							$url->frequency  =  $this->getMetaValue(null,$result->frequency,$defaults->frequency) ;		
							
							$this->addUrls($result->posts, $url);
							
						}
					}
				}
			}		

		}	
		
		function getAuthors($sitemapDefaults)
		{


			 
			$results = dataAccess::getAuthors($sitemapDefaults->dateField );
			

			if ($results ) {

				
			
				foreach( $results as $result ) {
					
					
				 //	wp_cache_add($result ->ID, $result , 'posts');
	
					$defaults = $sitemapDefaults->authors;					
			
					$exlcude = $this->getMetaValue(null, $result->exclude, $defaults->exclude) ;
					
					if ($exlcude != 2)
					{
						$pageUrl =   get_author_posts_url($result->ID, $result->user_nicename);	

						if ($this->isIncluded($pageUrl,$sitemapDefaults->excludeRules ))
						{
							
							$url = new mapItem();
							$url->location = $pageUrl;			
							$url->title = $result->display_name ;
						 	$url->description = "";
							$url->modified  =  $result->sitemapDate ;
							$url->priority =   $this->getMetaValue(null,$result->priority,$defaults->priority)  ;	
							$url->frequency  = $this->getMetaValue(null,$result->frequency,$defaults->frequency) ;
												
							$this->addUrls($result->posts, $url);
						}
					}
				}
			}		
		}		

		function getArchive($sitemapDefaults)
		{

		
			$results = dataAccess::getArchives($sitemapDefaults->dateField );
			
			if ($results ) {
				
	 
				foreach( $results as $result ) {
					
					$now = getdate();
				
					if($result->month == date("n") && $result->year == date("Y"))
					{
						$defaults = $sitemapDefaults->recentArchive;	
					}
					else
					{
						$defaults = $sitemapDefaults->oldArchive;
					}
			 
					$exlcude =   $defaults->exclude  ;
					$posts = $result->posts;
					
					$pageUrl = get_month_link( $result->year , $result->month) ;
				
					
					if ($exlcude != 2)
					{	
							
						if ($this->isIncluded($pageUrl,$sitemapDefaults->excludeRules ))
						{
							
							$url = new mapItem();
							$url->location = $pageUrl;		
							$url->title = date('F', strtotime("2012-$result->month-01")) . " | " . $result->year ;
							$url->description = "";
							$url->modified  =  $result->sitemapDate ;
							$url->priority =     $defaults->priority  ;	
							$url->frequency  =  $defaults->frequency ;					
							
							$this->addUrls($result->posts, $url);
						 
						}
					}
					
				}
			}		
			
			
		}		
		function addHomePage($sitemapDefaults)
		{
			
					$defaults = $sitemapDefaults->homepage;
					$pageUrl = get_bloginfo( 'url' );

	
					$exlcude = $defaults->exclude ;
					
					if ($exlcude != 2)
					{	
							
						if ($this->isIncluded($pageUrl,$sitemapDefaults->excludeRules ))
						{
							
							$url = new mapItem();
							$url->location = $pageUrl;		
							$url->title = "Home page" ;
							$url->description = get_option( 'blogdescription');;
							$url->modified  =  dataAccess::getLastModified() ;
							$url->priority =   $defaults->priority  ;	
							$url->frequency  = $defaults->frequency ;					
							
							$this->addUrls(0, $url);
						 
						}
					}
		}
	
		function getAllUrls()
		{
			
			$settings = get_option( "wpXSG_sitemapDefaults"   , new sitemapDefaults()   );
			
			$this->addHomePage($settings);
			$this->getPosts($settings, 0);
		 	$this->getTaxonomy($settings);
		 	$this->getAuthors($settings);
			$this->getArchive($settings);
			
		}
		
		function getNewUrls()
		{
			$settings = get_option( "wpXSG_sitemapDefaults"   , new sitemapDefaults()   );
			$this->getPosts($settings, 20);
			 
		}
		
		function render($sitemapType) 
		{		 
			$startTime =  microtime(true) ;
			$siteName = get_option('blogname');
		
			$limit = 0;
			if ($sitemapType == 'rssnew')
			{
				$sitemapType = "rss";
				$this->getNewUrls();
			}
			elseif ($sitemapType == 'xsl')
			{
				// don't load urls for xsl
			}
			else
			{
				 $this->getAllUrls();
			}
			
		
			$renderer = sitemapRenderer::getInstance($sitemapType);
			

		 	$renderer->render($this->urlsList);
			
			$time = core::getTimeBand($startTime);
		 	core::updateStatistics("Render", "Render" . $sitemapType, $time);
			
			exit;
		
			 
		}
	
		public static function renderCredit()
		{
			$globalSettings =   get_option( "wpXSG_global"   , new globalSettings()  );
		
			if ($globalSettings->smallCredit)
			{
				echo '<div id="xsgFooter">Generated by XmlSitemapGenerator.org -
						<a href="https://xmlsitemapgenerator.org/wordpress-sitemap-generator-plugin.aspx" 
								title="WordPress XML Sitemap Generator Plugin">
								WordPress XML Sitemap Generator Plugin</a>
					</div>';
				
			}
			

			
			
		}
 
		
	}

 

?>