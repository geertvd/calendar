
// Global killswitch
if (Drupal.jsEnabled) {
  $(document).ready(eventAutoAttach);
}

/**
 * Attaches the block update behaviour to links tagged with 'updateblock' class.
 */
function eventAutoAttach() {
  $("div.calendar-calendar a.active").click(function() {
    this.blockUpdater = new blockUpdater( $(this).parents(".view-content"), $(this).href().replace("mini=", "view=ajax&mini="), eventAutoAttach); 
    return false; 
    });

}

/**
 * create an instance of this object in the onClick handler for block update links.
 * 
 * could be separated into misc/blockupdater.js
 */

function blockUpdater(element,url,callback) {
  var blockUpdate = this;
  element.blockUpdate = this; 

  this.element = element;
  this.callback = callback;

  this.oldHTML = this.element.html();

  // Keep block at it's current width/height to make the update less disruptive
  this.styleHeight = $(element).height();
  this.styleWidth  = $(element).width();
  $(element).height(element.offsetHeight+"px");
  $(element).width(element.offsetWidth+"px");

  // Clear block contents
  $(element).html("");

  // Insert progressbar
  this.progress = new Drupal.progressBar('updateprogress');
  $(this.element).prepend(this.progress.element);

  var rel = this;
  var cancel = document.createElement("a");
  $(cancel).html("cancel").attr("alt","cancel").addClass("cancel-update")
    .href("#").bind("click", function() {
    rel.update("abort",undefined,blockUpdate);
    return false;
  });

  this.element.prepend($(cancel)); 

  this.dontUpdate = false;

  $(this).ajaxComplete(function(settings, request) {
     this.update(settings, request, this); 
  }); 
  /**
   * the cancel button doesnt work sometimes, probably the ajax process completes first, 
   * and despite the click event ajaxComplete will run, and this.dontUpdate 
   * will be false for that run 
   */

  $.ajax({
    type: "GET",
    url: url
  });
}

blockUpdater.prototype.update = function (result, xmlHttp, blockUpdate) {
  if(!blockUpdate.dontUpdate) {
    blockUpdate.element.height(blockUpdate.styleHeight); 
    blockUpdate.element.width(blockUpdate.styleWidth);

    if (result!=undefined && result!="abort") {
      blockUpdate.element.html(xmlHttp.responseText);
    }
    else if (result == "abort") {
      blockUpdate.element.html(this.oldHTML);
      blockUpdate.element.append("<p class='calendar-log'>Update aborted.</p>");
      blockUpdate.dontUpdate = true;
    }
    else {
      blockUpdate.element.html(this.oldHTML);
      blockUpdate.element.append("<p class='calendar-log'>Update failed</p>");
      blockUpdate.dontUpdate = true;
    }

    if(blockUpdate.callback != undefined)
      blockUpdate.callback();
  }
}