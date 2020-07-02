// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let settings   = {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function save() {
  data = { callback: 'save' };
  $.post('/post',data);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function updateSettings() {
  let [ hueMin, hueMax ] = $('#hueRange').val().split(';');
  let [ satMin, satMax ] = $('#satRange').val().split(',');
  let [ valMin, valMax ] = $('#valRange').val().split(',');
  let erosions = $('#erosions').val();
  let dilations = $('#dilations').val();

  let data = {
    callback: 'settings',
    hueMin,
    hueMax,
    satMin,
    satMax,
    valMin,
    valMax,
    erosions,
    dilations,
  };
  $.post('/post', data);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

$(document).ready(function() {
  $.get('/get/settings', function(data, status) {
    if (status === 'success') {
      bgSettings = JSON.parse(data);
      $('#hueRange').val(`${bgSettings.hueMin};${bgSettings.hueMax}`).trigger('change');
      $('#satRange').jRange('setValue',`${bgSettings.satMin},${bgSettings.satMax}`);
      $('#valRange').jRange('setValue',`${bgSettings.valMin},${bgSettings.valMax}`);
      $('#erosions').val(bgSettings.erosions);
      $('#dilations').val(bgSettings.dilations);
    }
  });

  $('input.nice-number').niceNumber();
  $('input.circle-range-select').lcnCircleRangeSelect();
  $('input.jquery-range').jRange({
    from: 0,
    to: 255,
    step: 1,
    isRange: true,
    theme: 'theme-config',
  });

  $('#hueRange').on('change',updateSettings);
  $('#satRange').on('change',updateSettings);
  $('#valRange').on('change',updateSettings);
  $('#erosions').siblings('button').on('click',updateSettings);
  $('#dilations').siblings('button').on('click',updateSettings);  
  

    /*window.setInterval( function() {
    $.get('/get/cameraImage',function(data, status) {
      $('#cameraImage').attr('src',`data:image/jpeg;base64,${data}`);
    });
    $.get('/get/ballMask',function(data, status) {
      $('#ballMaskImage').attr('src',`data:image/jpeg;base64,${data}`);
    });
    $.get('/get/bgMask',function(data, status) {
      $('#bgMaskImage').attr('src',`data:image/jpeg;base64,${data}`);
    });
  }, 100);*/
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
