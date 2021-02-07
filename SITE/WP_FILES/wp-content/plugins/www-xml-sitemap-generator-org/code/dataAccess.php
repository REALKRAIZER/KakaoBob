<?php

namespace xmlSitemapGenerator;





class dataAccess {
	
	
	
	function __construct() 
	{

	}
	
	static function execute($cmd) 
	{
		global $wpdb;
		$results = $wpdb->get_results($cmd, OBJECT );	

		return $results;
	}

	static function getDateField($name)
	{
		if ($name == "created")
		{ 
			return "post_date";
		}
		else
		{
			return "post_modified";
		}
	}
 

 
	public static function createMetaTable()
	{
		global $wpdb;		
		$tablemeta = $wpdb->prefix . 'xsg_sitemap_meta';
		$cmd = "CREATE TABLE IF NOT EXISTS `{$tablemeta}` (
				  `itemId` int(11) DEFAULT '0',
				  `inherit` int(11) DEFAULT '0',
				  `itemType` varchar(8) DEFAULT '',
				  `exclude` int(11) DEFAULT '0',
				  `priority` int(11) DEFAULT '0',
				  `frequency` int(11) DEFAULT '0',
				  UNIQUE KEY `idx_xsg_sitemap_meta_ItemId_ItemType` (`itemId`,`itemType`)
				) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='generatated by XmlSitemapGenerator.org';";

			
		$wpdb->query($cmd);

	}
	public static function getMetaItem($id, $type)
	{
		global $wpdb;
		$tablemeta = $wpdb->prefix . 'xsg_sitemap_meta';
		$cmd = " SELECT * FROM {$tablemeta}
				 WHERE itemId = %d AND itemType = %s ";
		
		$cmd = $wpdb->prepare($cmd, $id, $type);
		
		$settings = $wpdb->get_row($cmd);
		
		if (!$settings) {return new metaSettings(); }
 		
		return $settings ;
	}
	
 
	public static function saveMetaItem($metaItem)
	{
		global $wpdb;		
		$tablemeta = $wpdb->prefix . 'xsg_sitemap_meta';
		$cmd = " INSERT INTO {$tablemeta} (itemId, itemType, exclude, priority, frequency, inherit) 
				 VALUES(%d, %s, %d, %d, %d, %d) 
						ON DUPLICATE KEY UPDATE 
							exclude=VALUES(exclude), priority=VALUES(priority), frequency=VALUES(frequency), inherit=VALUES(inherit) ";
			
		
	 
		$itemId = $metaItem->itemId;
		$itemType = $metaItem->itemType;
		$exclude = $metaItem->exclude;
		$priority = $metaItem->priority;
		$frequency = $metaItem->frequency;
		$inherit = $metaItem->inherit;
	 		
		$cmd = $wpdb->prepare($cmd, $itemId, $itemType, $exclude, $priority , $frequency,$inherit);
		
		$settings = $wpdb->query($cmd);
	
	}

     static function getPostTypes()
	{
		$args = array(
		   'public'   => true,
		   '_builtin' => false
		);
		  
		$output = 'names'; // 'names' or 'objects' (default: 'names')
		$operator = 'and'; // 'and' or 'or' (default: 'and')
		  
		$post_types = get_post_types( $args, $output, $operator );	

		return $post_types;		
	}
	
	// type = "post" or "page" , date = "created" or "updated"
	//$limit = 0 for no limit.)
	public static function getPages($date  , $limit)
	{
		global $wpdb;
		$date = self::getDateField($date);
		$frontPageId = get_option( 'page_on_front' );
	
		$tablemeta = $wpdb->prefix . 'xsg_sitemap_meta';
		
		$postTypes = "";
		foreach ( self::getPostTypes()  as $post_type ) 
		{
			$postTypes .=  " OR post_type = '{$post_type}'";
		}
			
			
		$cmd = "SELECT 
				posts.*,   
				postmeta.*,  Tag_meta.* , UNIX_TIMESTAMP({$date}) as sitemapDate
				FROM {$wpdb->posts} as posts 
					LEFT JOIN {$tablemeta} as postmeta ON posts.Id = postmeta.ItemId AND postmeta.itemId
					LEFT JOIN 
							(SELECT  
								terms.object_id as Post_id,
								Max(meta.exclude) as tagExclude,
								Max(meta.priority) as tagPriority,
								Max(meta.frequency) as tagFrequency
							FROM {$tablemeta} as meta 
								INNER JOIN {$wpdb->term_relationships} as terms
								ON  meta.itemId = terms.term_taxonomy_id AND meta.itemType = 'posts'
							WHERE meta.itemType = 'taxonomy' AND meta.inherit = 1
								
							GROUP BY terms.object_id 
							) as Tag_meta
						ON posts.Id = Tag_meta.Post_id
				WHERE (post_status = 'publish' OR post_status = 'future' ) AND (post_type = 'page' OR  post_type = 'post' {$postTypes})   
					AND posts.post_password = ''  AND posts.ID <> {$frontPageId}
				ORDER BY {$date} DESC  ";
 
			
		if ($limit > 0 ) 
		{ 
			$cmd .= " LIMIT {$limit} " ; 
		}

	
	
		$results = self::execute($cmd);
		
		return $results;				
	}
	
	private static function getTaxonomyTypes()
	{
		$taxonomies =  get_taxonomies(array( "public" => "1",  "show_ui" =>"1" ), 'names' ,'and');	
		$taxonomies = "'" . implode("','", $taxonomies) . "'";
		return $taxonomies;
	}

	public static function  getTaxonomy($date = "updated"){

		global $wpdb;
		$taxonomies = self::getTaxonomyTypes();
		$date = self::getDateField($date);
		$tablemeta = $wpdb->prefix . 'xsg_sitemap_meta';
		$cmd = "SELECT  terms.term_id, terms.name, terms.slug, terms.term_group,
					tax.term_taxonomy_id,  tax.taxonomy, tax.description,   tax.description,
						meta.exclude, meta.priority, meta.frequency,
						UNIX_TIMESTAMP(Max(posts.{$date})) as sitemapDate,  Count(posts.ID) as posts
				  
				FROM {$wpdb->terms} as terms
					INNER JOIN {$wpdb->term_relationships} as Relationships ON terms.Term_id = Relationships.term_taxonomy_id
					INNER JOIN {$wpdb->posts} as posts ON Relationships.object_id = posts.Id
							AND posts.post_status = 'publish' AND posts.post_password = ''
					INNER JOIN {$wpdb->term_taxonomy} as tax ON terms.term_id = tax.term_id
					LEFT JOIN {$tablemeta} as meta ON terms.term_Id = meta.ItemId AND meta.itemType = 'taxonomy'
				WHERE tax.taxonomy IN ({$taxonomies})
				GROUP BY  terms.term_id, terms.name, terms.slug, terms.term_group, tax.description, tax.term_taxonomy_id,  tax.taxonomy, tax.description, meta.exclude, meta.priority, meta.frequency";
			
		$results = self::execute($cmd);
		 
		return $results;		

	}
 
	public static function  getAuthors($date = "updated") {

		global $wpdb;
		$date = self::getDateField($date);
		$tablemeta = $wpdb->prefix . 'xsg_sitemap_meta';
	
		$cmd = "SELECT users.ID, users.user_nicename, users.user_login, users.display_name ,meta.exclude, meta.priority, meta.frequency,
					UNIX_TIMESTAMP(MAX(posts.{$date})) AS sitemapDate, 	Count(posts.ID) as posts
				FROM {$wpdb->users} users LEFT JOIN {$wpdb->posts} as posts ON users.Id = posts.post_author 
						AND posts.post_type = 'post' AND posts.post_status = 'publish' AND posts.post_password = ''
				LEFT JOIN {$tablemeta} as meta ON users.ID = meta.ItemId AND meta.itemType = 'author'
				GROUP BY  users.user_nicename, users.user_login, users.display_name, meta.exclude, meta.priority, meta.frequency";

		$results = self::execute($cmd);
		 
		return $results;		

	}
		
	
	
	public static function  getArchives($date = "updated"){
		
		global $wpdb;
		
		$date = self::getDateField($date);
		
		$cmd = "SELECT DISTINCT YEAR(post_date) AS year,MONTH(post_date) AS month, 
					UNIX_TIMESTAMP(MAX(posts.{$date})) AS sitemapDate, 	Count(posts.ID) as posts
			FROM {$wpdb->posts} as posts
			WHERE post_status = 'publish' AND post_type = 'post' AND posts.post_password = ''
			GROUP BY YEAR(post_date), MONTH(post_date)";


		$results = self::execute($cmd);
		 
		return $results;	
						
	}

	
	public static function getLastModified($date = "updated")
	{
		 
		
		global $wpdb;
	
		$date = self::getDateField($date);
	 
		$cmd = "SELECT UNIX_TIMESTAMP(MAX({$date}))
				FROM {$wpdb->posts} as posts
				WHERE post_status = 'publish'";
			
		$date = $wpdb->get_var($cmd);
		 
		return $date;
	}
	
	
	public static function getPostCountBand()
	{
		
		global $wpdb;
	
		
		$cmd = "SELECT COUNT(*)
				FROM {$wpdb->posts} as posts
				WHERE post_status = 'publish'";
			
		$postCount = (int)$wpdb->get_var($cmd);
		 
		if( $postCount = 0) {$postCountLabel = "0";}
		else if( $postCount <= 10) {$postCountLabel = "1 to 10";}
		else if( $postCount <= 25) {$postCountLabel = "11 to 25";}
		else if ($postCount <= 50) {$postCountLabel = "26 to 50";}
		else if ($postCount <= 100) {$postCountLabel = "51 to 100";}
		else if ($postCount <= 500) {$postCountLabel = "101 to 500";}
		else if ($postCount <= 500) {$postCountLabel = "501 to 1000";}
		else if($postCount < 10000) {$postCountLabel = round($postCount / 1000) * 1000;}
		else if($postCount < 100000) {$postCountLabel = round($postCount / 10000) * 10000;}
		else {$postCountLabel = round($postCount / 100000) * 100000;}
		
		return $postCountLabel;
		
	}
 



	
	
}




?>