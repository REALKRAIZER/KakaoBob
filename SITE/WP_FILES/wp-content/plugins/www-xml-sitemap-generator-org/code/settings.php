<?php

namespace xmlSitemapGenerator;

// settings for generating a map


class settings
{
	
	public static function addHooks()
	{
		add_action('admin_menu', array(  __CLASS__, 'admin_menu' ) );
		add_action('admin_init', array( __CLASS__, 'register_settings' ) );
	}
	
	public static function admin_menu() 
	{
		add_options_page( 'XML Sitemap Settings','XML Sitemap','manage_options', XSG_PLUGIN_NAME , array( __CLASS__ , 'render' ) );
	}
	
	public static function register_settings()
	{
		register_setting( XSG_PLUGIN_NAME, XSG_PLUGIN_NAME );
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
	static function postTypeDefault($sitemapDefaults,$name)
	{
		
		return ( isset( $sitemapDefaults->{$name} ) ?  $sitemapDefaults->{$name} : $sitemapDefaults->posts );
	}



	
	static function getDefaults($name){
		
		$settings = new metaSettings();

		$settings->exclude = ( isset( $_POST[$name . 'Exclude'] ) ?  $_POST[$name . 'Exclude'] : 0 );
		$settings->priority  = ( isset( $_POST[$name . 'Priority'] ) ?  $_POST[$name . 'Priority'] : 0  );
		$settings->frequency  = ( isset( $_POST[$name . 'Frequency'] ) ?  $_POST[$name . 'Frequency'] : 0 );
		$settings->scheduled  = ( isset( $_POST[$name . 'Scheduled'] ) ?  1 : 0 );
		
		return $settings;
	}

	static function  handlePostBack(){
		
        if (!(strtoupper($_SERVER['REQUEST_METHOD']) == 'POST')){ return;  }

		/* Verify the nonce before proceeding. */
	 	if ( !isset( $_POST['wpXSG_meta_nonce'] ) || !wp_verify_nonce( $_POST['wpXSG_meta_nonce'], basename( __FILE__ ) ) )
	  	return ;
		
		
		if ( !current_user_can( 'manage_options') ) {return;}

		
			$globalSettings = new globalSettings();
			

			$register = ( isset( $_POST['register'] ) ?  $_POST['register'] : 0 );
			
			$globalSettings->addRssToHead  = ( isset( $_POST['addRssToHead'] ) ?  $_POST['addRssToHead'] : 0 );
			$globalSettings->pingSitemap = ( isset( $_POST['pingSitemap'] ) ?  $_POST['pingSitemap'] : 0 );
			$globalSettings->addToRobots = ( isset( $_POST['addToRobots'] ) ?  $_POST['addToRobots'] : 0 );
			$globalSettings->sendStats = ( isset( $_POST['sendStats'] ) ?  $_POST['sendStats'] : 0 );
			$globalSettings->smallCredit = ( isset( $_POST['smallCredit'] ) ?  $_POST['smallCredit'] : 0 );
			$globalSettings->robotEntries = ( isset( $_POST['robotEntries'] ) ?  $_POST['robotEntries'] : "" );
			$globalSettings->registerEmail = ( isset( $_POST['registerEmail'] ) ?  $_POST['registerEmail'] : "" );
			
			$globalSettings->urlXmlSitemap = ( isset( $_POST['urlXmlSitemap'] ) ?  $_POST['urlXmlSitemap'] : "xmlsitemap1.xml" );
			$globalSettings->urlRssSitemap = ( isset( $_POST['urlRssSitemap'] ) ?  $_POST['urlRssSitemap'] : "rsssitemap.xml" );
			$globalSettings->urlRssLatest = ( isset( $_POST['urlRssLatest'] ) ?  $_POST['urlRssLatest'] : "rsslatest.xml" );
			$globalSettings->urlHtmlSitemap = ( isset( $_POST['urlHtmlSitemap'] ) ?  $_POST['urlHtmlSitemap'] : "htmlsitemap.htm" );
			
			$globalSettings->register = $register;
			

			// if new register staturs then .....
			if ($register != get_option('wpXSG_registered'))
			{
				$postData = array(
					'email' => $globalSettings->registerEmail,
					'website' => get_bloginfo( 'url' ),
					'register' => $register,
					'id' => get_option('wpXSG_MapId')
					);
				$url = 'https://xmlsitemapgenerator.org/services/WordpressOptIn.aspx';
			//	$url = 'http://localhost:51944/services/WordpressOptIn.aspx';
				try
				{
					$response = wp_remote_post($url,
						array(
							'method' => 'POST',
							'body' => $postData
							));		
				} 
				catch (Exception $e) 
				{}
			}
			
			update_option( "wpXSG_global" ,  $globalSettings , true);	
			update_option( "wpXSG_registered" ,  $register , true);
			
		flush_rewrite_rules();
		
		
			$sitemapDefaults = new sitemapDefaults();
			
			$sitemapDefaults->dateField = ( isset( $_POST['dateField'] ) ?  $_POST['dateField'] : $sitemapDefaults->dateField );
			$sitemapDefaults->homepage  = self::getDefaults("homepage");
			$sitemapDefaults->pages = self::getDefaults("pages");
			$sitemapDefaults->posts = self::getDefaults("posts");
			$sitemapDefaults->taxonomyCategories = self::getDefaults("taxonomyCategories");
			$sitemapDefaults->taxonomyTags = self::getDefaults("taxonomyTags");
		 
			$sitemapDefaults->recentArchive = self::getDefaults("recentArchive");
			$sitemapDefaults->oldArchive  = self::getDefaults("oldArchive");
			$sitemapDefaults->authors  = self::getDefaults("authors");
			
			$sitemapDefaults->excludeRules = ( isset( $_POST['excludeRules'] ) ?  $_POST['excludeRules'] : "" );
			
			foreach ( self::getPostTypes()  as $post_type ) 
			{
				$sitemapDefaults->{$post_type}  = self::getDefaults($post_type);
			}
			
			
			 
			
		 	update_option( "wpXSG_sitemapDefaults" ,  $sitemapDefaults , false);
			
			core::updateStatistics("Admin", "SaveSettings",0);
			
     
}
 
	static function RenderDefaultSection($title,$name,$defaults,$scheduled){
		
			?>
							
							<tr>
								<td scope="col"><?php echo $title; ?></td>
								<td scope="col"><select  name="<?php echo $name; ?>Exclude" id="<?php echo $name; ?>Exclude" ></select> </td>
								<td scope="col"><select  name="<?php echo $name; ?>Priority" id="<?php echo $name; ?>Priority" ></select>   </td>
								<td scope="col"><select  name="<?php echo $name; ?>Frequency" id="<?php echo $name; ?>Frequency" ></select>  </td>
							<?php	if ($scheduled) { ?>
								<td scope="col"><input type="checkbox"  name="<?php echo $name; ?>Scheduled" id="<?php echo $name; ?>Scheduled"  
									<?php	if ($defaults->scheduled) { echo 'checked="checked"';} ?>
								></input>  </td>
							<?php	}
								else
								{ echo '<td scope="col"></td>';}
							 ?>
							</tr>
							<script>
                xsg_populate("<?php echo $name; ?>Exclude" ,excludeDefaults, <?php echo $defaults->exclude  ?>);
                xsg_populate("<?php echo $name; ?>Priority" ,priorityDefaults, <?php echo $defaults->priority  ?>);
                xsg_populate("<?php echo $name; ?>Frequency" ,frequencyDefaults, <?php echo $defaults->frequency  ?>);
		 
							</script>
			
			<?php

	}
	
	
 
 
	public static function render()
	{
		 
		self::handlePostBack();
		
		$globalSettings =   core::getGlobalSettings();
		$sitemapDefaults =  get_option( "wpXSG_sitemapDefaults"   , new sitemapDefaults()  );
		
		core::updateStatistics("Admin", "ViewSettings",0);
 
		?>



		
<form method="post"  > 

   <?php 	wp_nonce_field( basename( __FILE__ ), 'wpXSG_meta_nonce' ); ?>
		
		
<div class="wrap" >

        <h2>Google XML Sitemap Generator</h2>

 

		<p>Here you can edit your admin settings and defaults. You can override categories, tags, pages and posts when adding and editing them.</p>
		<p>Please support us with a <a target="_blank" href="<?php echo XSG_DONATE_URL ?>">small donation</a>. If you have any comments, questions,
		suggestions and bugs please <a target="_blank" href="https://xmlsitemapgenerator.org/contact.aspx">contact us</a>.</strong></p>
		
<div id="poststuff" class="metabox-holder has-right-sidebar">

            <div class="inner-sidebar">
                <div   class="meta-box-sortabless ui-sortable" style="position:relative;">

                    <div  class="postbox">
                        <h3 class="hndle"><span>Sitemap related urls</span></h3>
                        <div class="inside">
						<p>Pages that are created or modified by Xml Sitemap Generator</p>
                            <ul>
		<?php  
			$url = get_bloginfo( 'url' ) ;
		
			echo '<li><a target="_blank" href="' . $url .'/' . $globalSettings->urlXmlSitemap . '">XML Sitemap</a></li>';
			echo '<li><a target="_blank" href="' . $url .'/' . $globalSettings->urlRssSitemap . '">RSS Sitemap</a></li>';
			echo '<li><a target="_blank" href="' . $url .'/' . $globalSettings->urlRssLatest . '">New Pages RSS</a></li>';
			echo '<li><a target="_blank" href="' . $url .'/' . $globalSettings->urlHtmlSitemap . '">HTML Sitemap</a></li>';
			echo '<li><a target="_blank" href="' . $url .'/robots.txt">Robots.txt</a></li>';
		
		?>
		
							</ul>
               
                        </div>
                    </div>
					
					<div  class="postbox">
                        <h3 class="hndle"><span>Please support us</span></h3>
                        <div class="inside">
						<p>We take time out of our personal lives to develop and support our sitemap tools and cover costs out of our own pockets. </p>
						<p>Please support us with a <a target="_blank" href="<?php echo XSG_DONATE_URL ?>">small donation</a>.</p>
						<p>Please feel free to <a target="_blank" href="https://xmlsitemapgenerator.org/contact.aspx">contact us</a> with any comments,
							questions, suggestions and bugs.</strong></p>

                        </div>
						
                    </div>
					
					
                    <div  class="postbox">
                        <h3 class="hndle"><span>Webmaster tools</span></h3>
                        <div class="inside">
						<p>It is highly recommended you register your sitemap 
						with webmaster tools to obtain performance insights.</p>
                            <ul>
								<li><a href="https://www.google.com/webmasters/tools/">Google Webmaster tools</a></li>
								<li><a href="http://www.bing.com/toolbox/webmaster">Bing Webmaster tools</a></li>
								<li><a href="http://zhanzhang.baidu.com/">Baidu Webmaster tools</a></li>
								<li><a href="https://webmaster.yandex.com/">Yandex Webmaster tools</a></li>
								
							</ul>
               
                        </div>
                    </div>
				
				
                    <div  class="postbox">
                        <h3 class="hndle"><span>Useful links</span></h3>
                        <div class="inside">
                            <ul>
							<li><a href="https://xmlsitemapgenerator.org/Wordpress-sitemap-plugin.aspx">Help and support</a></li>
								<li><a href="http://blog.xmlsitemapgenerator.org/">blog.XmlSitemapGenerator.org</a></li>
								<li><a href="https://twitter.com/createsitemaps">twitter : @CreateSitemaps</a></li>
								<li><a href="https://www.facebook.com/XmlSitemapGenerator">facebook XmlSitemapGenerator</a></li>
		
							</ul>
               
                        </div>
                    </div>

					

					
                </div>
            </div>

		
			
 <div class="has-sidebar">
 

					
<div id="post-body-content" class="has-sidebar-content">
				
	<div class="meta-box-sortabless">

			<script type="text/javascript" src="<?php echo xsgPluginPath(); ?>scripts.js"></script>
			
			
			
	<div  class="postbox" <?php if (!get_option('wpXSG_registered')) {echo 'style="border-left:solid 4px #dc3232;"';} ?>">
		<h3 class="hndle"><span>Register for important updates</span></h3>
		<div class="inside">


				<?php if (!get_option('wpXSG_registered')) {echo "<p><strong>Please ensure you register for important updates to stay up-to-date.</strong></p>";} ?> 


				<p>
					<input type="checkbox" name="register" id="register" value="1" <?php checked($globalSettings->register, '1'); ?> /> Recieve important news and updates about the plugin. <a href="/help/privacy.aspx" target="_blank" rel="nofollow">Privacy Policy</a>.
				</p>
			<table><tr><td>
				<p>
					<label for="email"   >Email address</label><br />
					<input type="text" name="registerEmail" size="40" value="<?php echo core::safeRead2($globalSettings,"registerEmail",get_option( 'admin_email' ) ); ?>" />
				</p> 
			</td><td>&nbsp;</td><td>  
				<p>
					<label for="website">Website</label><br />
					<input type="text" name="website" size="40" readonly value="<?php echo site_url(); ?>" />
				</p>
			</td></tr></table>
		</div>
	</div> 


		
	<div  class="postbox">
		<h3 class="hndle"><span>Output urls</span></h3>
		<div class="inside">
				<p>You can output URLs for the various sitemap files using the settings below.</p>
				<p>Caution should be take to avoid conflicts with other plugins which might output similar files. Please ensure it is a simple filename with no slashes and only one dot.</p>
				<table><tr><td>
					<p>
						<label for="email"   >XML Sitemap URL</label><br />
						<input type="text" name="urlXmlSitemap" size="40" value="<?php echo core::safeRead($globalSettings,"urlXmlSitemap"); ?>" />
					</p> 
					<p>
						<label for="email"   >RSS Sitemap URL</label><br />
						<input type="text" name="urlRssSitemap" size="40" value="<?php echo core::safeRead($globalSettings,"urlRssSitemap"); ?>" />
					</p> 
				</td><td>&nbsp;</td><td>
					<p>
						<label for="email"   >RSS Latest URLs</label><br />
						<input type="text" name="urlRssLatest" size="40" value="<?php echo core::safeRead($globalSettings,"urlRssLatest"); ?>" />
					</p> 
					<p>
						<label for="email"   >HTML Sitemap URL</label><br />
						<input type="text" name="urlHtmlSitemap" size="40" value="<?php echo core::safeRead($globalSettings,"urlHtmlSitemap"); ?>" />
					</p> 
				</td></tr></table>
		</div>
	</div> 
		
	<div  class="postbox">
		<h3 class="hndle"><span>General settings</span></h3>
		<div class="inside">

               
				<p>General options for your sitemap. 
				We recommend you enable all of these.</p>

					<ul>
						<li>
							<input type="checkbox" name="pingSitemap" id="pingSitemap" value="1" <?php checked($globalSettings->pingSitemap, '1'); ?> /> 
							<label for="sm_b_ping">Automatically ping Google / Bing (MSN & Yahoo) daily</label><br>
						</li>
						<li>
							<input type="checkbox" name="addRssToHead" id=""addRssToHead" value="1" <?php checked($globalSettings->addRssToHead, '1'); ?> />
							<label for="sm_b_ping">Add latest pages / post RSS feed to head tag</label><br>
						</li>
						<li>
							<input type="checkbox" name="addToRobots" id="addToRobots" value="1" <?php checked($globalSettings->addToRobots, '1'); ?> />
							<label for="sm_b_ping">Add sitemap links to your robots.txt file</label><br>
						</li>
						<li>
							<input type="checkbox" name="sendStats" id="sendStats" value="1" <?php checked($globalSettings->sendStats, '1'); ?> />
							<label for="sm_b_ping">Help us improve by allowing basic usage stats (Page count, PHP Version, feature usage, etc.)</label><br>
						</li>
						<li>
							<input type="checkbox" name="smallCredit" id="smallCredit" value="1" <?php checked($globalSettings->smallCredit, '1'); ?> />
							<label for="sm_b_ping">Support us by allowing a small credit in the sitemap file footer (Does not appear on your website)</label><br>
						</li>
					</ul>
		</div>
	</div> 

	<div  class="postbox">
		<h3 class="hndle"><span>Sitemap defaults</span></h3>
		<div class="inside">
		
               
				<p>Set the defaults for your sitemap here.</p>
				
				<ul>
										<li>
							<select name="dateField" id="dateField">
								<option  <?php  if ($sitemapDefaults->dateField == "created") {echo 'selected="selected"';} ?>>created</option>
								<option <?php  if ($sitemapDefaults->dateField == "updated") {echo 'selected="selected"';} ?>>updated</option>
							</select>
							<label for="sm_b_ping">  date field to use for modified date / recently updated.</label><br>
						</li>
					</ul>
					
				<p>You can override the sitemap default settings for taxonomy items (categories, tags, etc), pages and posts when adding and editing them.</p>
		
						<table class="wp-list-table widefat fixed striped tags" style="clear:none;">
							<thead>
							<tr>
								<th scope="col">Page / area</th>
								<th scope="col">Exclude</th>
								<th scope="col">Relative priority</th>
								<th scope="col">Update frequency</th>
								<th scope="col">Include scheduled</th>
							</tr>
							</thead>
							<tbody id="the-list" >
							
							
<?php 

		self::RenderDefaultSection("Home page","homepage",$sitemapDefaults->homepage, false);
		self::RenderDefaultSection("Regular page","pages",$sitemapDefaults->pages, true);
		self::RenderDefaultSection("Post page","posts",$sitemapDefaults->posts, true);
		self::RenderDefaultSection("Taxonomy - categories","taxonomyCategories",$sitemapDefaults->taxonomyCategories, false);
		self::RenderDefaultSection("Taxonomy - tags","taxonomyTags",$sitemapDefaults->taxonomyTags, false);
 
		self::RenderDefaultSection("Archive - recent","recentArchive",$sitemapDefaults->recentArchive, false);
		self::RenderDefaultSection("Archive - old","oldArchive",$sitemapDefaults->oldArchive, false);
		self::RenderDefaultSection("Authors","authors",$sitemapDefaults->authors, false);
?>
</table>

<p>Custom post types<p/>
<table class="wp-list-table widefat fixed striped tags" style="clear:none;">
							<thead>
							<tr>
								<th scope="col">Page / area</th>
								<th scope="col">Exclude</th>
								<th scope="col">Relative priority</th>
								<th scope="col">Update frequency</th>
								<th scope="col">Include scheduled</th>
							</tr>
							</thead>
							
<?php 
		foreach ( self::getPostTypes()  as $post_type ) 
		{
			self::RenderDefaultSection($post_type,$post_type, self::postTypeDefault($sitemapDefaults,$post_type), true);
		}
 ?>
							
						
				
							
						</tbody></table>
                		 
          </div>
	</div>

		<div  class="postbox">
		<h3 class="hndle"><span>Robots.txt</span></h3> 
		<div class="inside">
			 <p>Add custom entries to your robots.txt file.</p>  		 
			<textarea name="robotEntries" id="robotEntries" rows="10" style="width:98%;"><?php echo core::safeRead($globalSettings,"robotEntries"); ?></textarea>
			
		</div>
	</div> 
	
<?php submit_button(); ?>


	<div  class="postbox">
		<h3 class="hndle"><span>Log</span></h3> 
		<div class="inside">
			   		 
			<?php echo	core::getStatusHtml();?>
		</div>
	</div> 
				
			

</div>

</div>



</div>
 
		


</div>
</div>


</form>






	<?php 

	}
	

	
}	 




?>