if (Drupal.jsEnabled) {
  $(document).ready(function() {
    // add the attribute rel=facebox to all of the links I want to have a popup
    $('div.view-field > a').attr('class', 'popup');
   
    // on click of a link
    $('a.popup').click(function(a) {
    
      // Get NodeID and ItemID
      var ids = $(this).parent().parent().parent().attr('id');
      var arr = ids.split(":");
      var nid = arr[1];
      var id = arr[4];
       
      // If there is no nid (like for a remote node), don't intervene.     
      if (!nid) {
        return true;
      }
      // Make sure that other stuff is closed. This seems to cause a problem in Fx2 and IE7.
      $('div#calpopup').fadeOut();
      $('div#calpopup').remove();
      
      // create div to hold data and add it to the end of the body
      var div = $('<div id="calpopup"><div id="popup-close"><img id="popup-close-img" src="' + var_path + '/images/cross.png" /></div><div id="calpopup-body"><img src="' + var_path + '/images/throbber.gif" id="popthrobber" /></div></div>').attr('style','display: none');
      div.appendTo(document.body);
      
      // Locate Popup
      // This is a hack. In D6 do it right with $.css.offset()
      var x = a.pageX;
      var y = a.pageY;
      $('#calpopup').css('left', (x - 100) + "px");
      $('#calpopup').css('top', (y + 25) + "px");
      
      // Show Popup
      $('#calpopup').fadeIn('slow');
      
      // If sucessful call this
      function domCallback(msg) {
        $('#calpopup-body').html(msg);
      }
      
      // fill the div with data
      $.ajax({
        type: "GET",
        url: var_base_path + "?q=jcalendar/getnode/"+nid+"/"+ids,
        success: function(msg){
          domCallback(msg);
        }
      });
      
      // On click of the close image
      $('img#popup-close-img').click(function(x) {
        $('div#calpopup').fadeOut();
        $('div#calpopup').remove();
      });
      
      $(document).click(function(y) {
        var $tgt = $(y.target);
        if (!$tgt.parents().is('div#calpopup')) {
          $('div#calpopup').fadeOut();
          $('div#calpopup').remove();
          $(document).unbind("click");
        }
      });
      
      // Don't Follow the real link
      return false;
    });
  });
}
