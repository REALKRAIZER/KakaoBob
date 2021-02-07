<?php
namespace xmlSitemapGenerator;

include_once 'dataAccess.php';

class categoryMetaData
{
 
	public static function addHooks()
	{
		$taxonomy = ( isset( $_REQUEST['taxonomy'] ) ?  $_REQUEST['taxonomy'] : '0' );
		add_action($taxonomy . '_edit_form',  array( __CLASS__, 'renderEdit' ) );
	    add_action($taxonomy . '_add_form_fields', array( __CLASS__, 'renderAdd' ) );
		add_action( 'created_' . $taxonomy, array( __CLASS__, 'save_metaData' ), 10, 2);
		add_action( 'edited_' . $taxonomy , array( __CLASS__, 'save_metaData' ), 10, 2); 
		
	}

 
	static function save_metaData( $term_id, $ttId ) {
		
		/* Verify the nonce before proceeding. */
	 	if ( !isset( $_POST['wpXSG_meta_nonce'] ) || !wp_verify_nonce( $_POST['wpXSG_meta_nonce'], basename( __FILE__ ) ) )
	  	return ;

	 
		/* Check if the current user has permission to edit the post. */
		if ( !current_user_can( 'manage_categories') )
		return $term_id;

 
	 	$settings = new metaSettings();
		
		$settings->id =  ( isset( $_POST['wpXSG-metaId'] ) ?  $_POST['wpXSG-metaId'] : '0' );
	 	$settings->itemId = $term_id ;
		$settings->itemType = "taxonomy";
	 	$settings->exclude = ( isset( $_POST['wpXSG-Exclude'] ) ?  $_POST['wpXSG-Exclude'] : '0' );
	 	$settings->priority = ( isset( $_POST['wpXSG-Priority'] ) ?  $_POST['wpXSG-Priority'] : 'default' );
	  	$settings->frequency = ( isset( $_POST['wpXSG-Frequency'] ) ?  $_POST['wpXSG-Frequency'] : 'default' );
		$settings->inherit = ( isset( $_POST['wpXSG-Inherit'] ) ?  $_POST['wpXSG-Inherit'] : 0 );
			
		dataAccess::saveMetaItem($settings );

		
	
	}
	
	
	
 	static function renderAdd($term_id  ) 
	{
		self::addHooks();
			
		$settings = new metaSettings(1,1,1);
		
	  wp_nonce_field( basename( __FILE__ ), 'wpXSG_meta_nonce' );
		?>
 
	<h3>Sitemap settings</h3>
	
	<p>Sitemap settings can be setup for individual categories/tags overriding the global settings. Category/tag settings will be inherited by related posts.<br /><br /></p>
	
	 <div class="form-field term-description-wrap">
		
		<label for="wpXSG-Exclude">Sitemap inclusion</label>
		<select  name="wpXSG-Exclude" id="wpXSG-Exclude" ></select>

		<p>Exclude this category/tag from your sitemap.</p>
	</div>

	 <div class="form-field term-description-wrap">
		<label for="wpXSG-Priority">Relative priority</label>
		<select  name="wpXSG-Priority" id="wpXSG-Priority" ></select>
		<p>Relative priority for this category/tag.</p>
	</div>
	
	<div class="form-field term-description-wrap">
		<label for="wpXSG-Frequency">Update frequency</label>
		<select  name="wpXSG-Frequency" id="wpXSG-Frequency" ></select>
		<p>Sitemap update frequency for this category/tag .</p>
	</div>

	<div class="form-field term-description-wrap">
		<label for="wpXSG-Inherit">Posts inheritance</label>
		<select  name="wpXSG-Inherit" id="wpXSG-Inherit" ></select>
		<p>Immediate child posts/pages inherit these settings.</p>
	</div>
	
<script type="text/javascript" src="<?php echo xsgPluginPath(); ?>scripts.js"></script>
<script>
  xsg_populate("wpXSG-Exclude" ,excludeSelect, <?php echo $settings->exclude  ?>);
  xsg_populate("wpXSG-Priority" ,prioritySelect, <?php echo $settings->priority  ?>);
  xsg_populate("wpXSG-Frequency" ,frequencySelect, <?php echo $settings->frequency  ?>);
  xsg_populate("wpXSG-Inherit" ,inheritSelect, <?php echo $settings->inherit  ?>);
</script>
		
		<?php
	
	}

	static function renderEdit($tag ) 
	{
		$term_id = $tag->term_id;
		self::addHooks();
		
		$settings =  dataAccess::getMetaItem($term_id , "taxonomy");  
 
	 
			wp_nonce_field( basename( __FILE__ ), 'wpXSG_meta_nonce' );
		?>

		
		<h3>Sitemap settings : 	 </h3>
	


		
		<table class="form-table">
		<tbody><tr class="form-field form-required term-name-wrap">
			<th scope="row"><label for="name">Sitemap inclusion</label></th>
			<td>
				<select  name="wpXSG-Exclude" id="wpXSG-Exclude" ></select>
				<p> Exclude this category/tag from your sitemap.</p>
			</td>
		</tr>
		<tr class="form-field term-slug-wrap">
			<th scope="row"><label for="slug">Relative priority</label></th>
			<td>
				<select  name="wpXSG-Priority" id="wpXSG-Priority" ></select>
				<p>Relative priority for this category/tag and related posts.</p>
			</td>
		</tr>
		<tr class="form-field term-description-wrap">
			<th scope="row"><label for="description">Update frequency</label></th>
			<td>
			<select  name="wpXSG-Frequency" id="wpXSG-Frequency" ></select>
			<p>Sitemap update frequency for this category/tag.</p>
			</td>
		</tr>
		<tr class="form-field term-description-wrap">
			<th scope="row"><label for="description">Post inheritance</label></th>
			<td>
			<select  name="wpXSG-Inherit" id="wpXSG-Inherit" ></select>
			<p>Immediate child posts/pages inherit these settings.</p>
			</td>
		</tr>
		</tbody></table>

<script type="text/javascript" src="<?php echo xsgPluginPath(); ?>scripts.js"></script>
<script>
  xsg_populate("wpXSG-Exclude" ,excludeSelect, <?php echo $settings->exclude  ?>);
  xsg_populate("wpXSG-Priority" ,prioritySelect, <?php echo $settings->priority  ?>);
  xsg_populate("wpXSG-Frequency" ,frequencySelect, <?php echo $settings->frequency  ?>);
  xsg_populate("wpXSG-Inherit" ,inheritSelect, <?php echo $settings->inherit  ?>);
</script>


		<?php
	
	}
 


		
 

}

?>