// $Id$
/**
 * Implementation of hook_elements.
 * 
 * Much of the colorpicker code was adapted from the Colorpicker module.
 * That module has no stable release yet nor any D6 branch.
 */

/*
 *  Bind the colorpicker event to the form element
 */
$(document).ready(function () {

  // do we have multiple calendar_colors?
  if ($("div.calendar_colorpicker").size() > 1) {
  
    // loop over each calendar_color type
    $("div.calendar_colorpicker").each(function() {

      // create the farbtastic colorpicker
    var farb = $.farbtastic(this);
    
    // get the id of the current matched colorpicker wrapper div
    var id = $(this).attr("id");

    // get the calendar_color_textfields associated with this calendar_color
    $("input.calendar_colorfield").filter("." + id).each(function () {
      // set the background colors of all of the textfields appropriately
       farb.linkTo(this);
    
      // when clicked, they get linked to the farbtastic colorpicker that they are associated with
      $(this).click(function () {
        farb.linkTo(this);
      });

    });

    });
  }
  else {
    // we do this differently because we don't care about the id
  var farb = $.farbtastic("div.calendar_colorpicker");
    $("input.calendar_colorfield").each(function () {
      // set the background colors of all of the textfields appropriately
      farb.linkTo(this);

      // update the farbtastic colorpicker when this textfield is clicked
      $(this).click(function () {
        farb.linkTo(this);
      });

    
  });
  }
});

