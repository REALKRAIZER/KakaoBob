jQuery(document).ready(function($){
  var language_display = $('input[name=googlelanguagetranslator_language_option]:checked').val();
  var display = $('select[name=googlelanguagetranslator_display] option:selected').val();
  var flag_display = $('input[name=googlelanguagetranslator_flags]:checked').val();
  var floating_widget_display = $('select[name=googlelanguagetranslator_floating_widget] option:selected').val();
	
  if(floating_widget_display == 'yes') {
    $('.floating-widget').removeClass('hidden');
  } else {
	$('.floating-widget').addClass('hidden');
  }
	
  $('input[name=googlelanguagetranslator_flags]').change(function(){
    if($(this).val() == 'hide_flags'){
      $('.choose_flags').fadeOut("slow");
    } else if ($(this).val() == 'show_flags') {
      $('.choose_flags').fadeIn("slow");
    }
  });

  //FadeIn and FadeOut Floating Widget Text setting
  $('select[name=googlelanguagetranslator_floating_widget]').change(function() {
    if($(this).val()=='yes') {
      $('.floating-widget').removeClass('hidden');
    } else {
      $('.floating-widget').addClass('hidden');
    }
  });
  
  //FadeIn and FadeOut Google Analytics tracking settings
  $('input[name=googlelanguagetranslator_analytics]').change(function() {
    var analytics = $(this);
    if(analytics.is(':checked')) {
      $('.analytics').fadeIn("slow");
    } else {
      $('.analytics').fadeOut("slow");
    }
  });
  
  //Hide or show Google Analytics ID field upon browser refresh  
  var analytics = $('input[name=googlelanguagetranslator_analytics]');
  if (analytics.is(':checked') )  {
    $('.analytics').css('display','');
  } else {
    $('.analytics').css('display','none');
  }
  
  //Prevent the translator preview from translating Dashboard text
  $('#adminmenu').addClass('notranslate');
  $('#wp-toolbar').addClass('notranslate');
  $('#setting-error-settings_updated').addClass('notranslate');
  $('.update-nag').addClass('notranslate');
  $('title').addClass('notranslate');
  $('#footer-thankyou').addClass('notranslate');
}); //jQuery

jQuery(document).ready(function($) { 
  $("#sortable,#sortable-toolbar").sortable({ 
    opacity: 0.7,
    distance: 10, 
    helper: "clone", 
    forcePlaceholderSize:true,
    update: function(event,ui) {
      var newOrder = $(this).sortable('toArray').toString();
        $.post("options.php",{order: newOrder});
	$('#order').val(newOrder);
    },
  });
  
  $("#sortable,#sortable-toolbar").disableSelection();
});

//Color Picker
jQuery(document).ready(function($) {
  $(function() {
    $('.color-field').wpColorPicker();
  });  
}); //jQuery