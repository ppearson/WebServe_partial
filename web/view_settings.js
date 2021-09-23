/*
 WebServe
 Copyright 2018-2019 Peter Pearson.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

function loadViewSettings(prefix)
{
    var thumbnailSizeElement = document.getElementById(prefix + "thumbnailSize");
    var thumbnailSizeIndexCookieValue = getCookie(prefix + "thumbnailSizeIndex");
    if (thumbnailSizeIndexCookieValue != "")
    {
        var thumbnailSizeIndexCookieValueInt = parseInt(thumbnailSizeIndexCookieValue, 10);
        thumbnailSizeElement.selectedIndex = thumbnailSizeIndexCookieValueInt;
    }
    else
    {
        // provide a default
        thumbnailSizeElement.selectedIndex = 1;
    }

    var galleryLinkToSlideshowToggleElement = document.getElementById(prefix + "galleryLinkToSlideshow");
    var galleryLinkToSlideshowCookieVal = getCookie(prefix + "galleryLinkToSlideshow");
    if (galleryLinkToSlideshowCookieVal == "1" || galleryLinkToSlideshowCookieVal == "")
    {
        galleryLinkToSlideshowToggleElement.checked = true;
    }

    var lazyLoadingToggleElement = document.getElementById(prefix + "lazyLoading");
    var lazyLoadingCookieVal = getCookie(prefix + "lazyLoading");
    if (lazyLoadingCookieVal == "1" || lazyLoadingCookieVal == "")
    {
        lazyLoadingToggleElement.checked = true;
    }

    var sortOrderElement = document.getElementById(prefix + "sortOrder");
    // this one might not exist in some cases
    if (sortOrderElement != null)
    {
        var sortOrderIndexCookieValue = getCookie(prefix + "sortOrderIndex");
        if (sortOrderIndexCookieValue != "")
        {
            var sortOrderIndexCookieValueInt = parseInt(sortOrderIndexCookieValue, 10);
            sortOrderElement.selectedIndex = sortOrderIndexCookieValueInt;
        }
        else
        {
            // provide a default
            sortOrderElement.selectedIndex = 1;
        }
    }

    var droneTypeToggleElement = document.getElementById(prefix + "typeDrone");
    var slrTypeToggleElement = document.getElementById(prefix + "typeSLR");
    var droneTypeCookieVal = getCookie(prefix + "typeDrone");
    if (droneTypeCookieVal == "1")
    {
        droneTypeToggleElement.checked = true;
    }
    var slrTypeCookieVal = getCookie(prefix + "typeSLR");
    if (slrTypeCookieVal == "1" || slrTypeCookieVal == "")
    {
        slrTypeToggleElement.checked = true;
    }

    var viewRatingCookieValue = getCookie(prefix + "viewRating");
    if (viewRatingCookieValue != "" && viewRatingCookieValue != "0")
    {
        var radioItems = document.getElementsByName(prefix + "viewRating");
        var intRatingIndex = parseInt(viewRatingCookieValue, 10); // zero-based
        if (intRatingIndex < radioItems.length - 1)
        {
            radioItems[intRatingIndex].checked = true;
        }
    }
}

function toggleViewSettingsPopupPanel(prefix)
{
    if (document.getElementById("viewSettingsPanel").style.display == "none" ||
        document.getElementById("viewSettingsPanel").style.display == "") {
        
        var screenInfo = "Screen resolution: " + screen.width + " x " + screen.height + "." +
            " Scaling factor: " + window.devicePixelRatio;

        document.getElementById("screenRes").innerHTML = screenInfo;

        document.getElementById("viewSettingsPanel").style.display = "block";
    }
    else {
        document.getElementById("viewSettingsPanel").style.display = "none";
    }
}

function closeViewSettingsPopupPanel(prefix)
{
    document.getElementById("viewSettingsPanel").style.display = "none";
}

function applyViewSettingsPopupPanel(prefix)
{
    var thumbnailSizeElement = document.getElementById(prefix + "thumbnailSize");
    var thumbnailSizeIndex = thumbnailSizeElement.selectedIndex;
    var thumbnailSizeValue = thumbnailSizeElement.options[thumbnailSizeIndex].text;
    setCookie(prefix + "thumbnailSizeIndex", thumbnailSizeIndex.toString(), 128);
    setCookie(prefix + "thumbnailSizeValue", thumbnailSizeValue, 128);

    var galleryLinkToSlideshowToggleElement = document.getElementById(prefix + "galleryLinkToSlideshow");
    if (galleryLinkToSlideshowToggleElement.checked == true) {
        setCookie(prefix + "galleryLinkToSlideshow", "1", 128);
    }
    else {
        setCookie(prefix + "galleryLinkToSlideshow", "0", 128);
    }

    var lazyLoadingToggleElement = document.getElementById(prefix + "lazyLoading");
    if (lazyLoadingToggleElement.checked == true) {
        setCookie(prefix + "lazyLoading", "1", 128);
    }
    else {
        setCookie(prefix + "lazyLoading", "0", 128);
    }

    var sortOrderElement = document.getElementById(prefix + "sortOrder");
    if (sortOrderElement != null)
    {
        var sortOrderIndex = sortOrderElement.selectedIndex;
        setCookie(prefix + "sortOrderIndex", sortOrderIndex.toString(), 128);
    }

    var droneTypeToggleElement = document.getElementById(prefix + "typeDrone");
    var slrTypeToggleElement = document.getElementById(prefix + "typeSLR");

    if (droneTypeToggleElement.checked == true) {
        setCookie(prefix + "typeDrone", "1", 128);
    }
    else {
        setCookie(prefix + "typeDrone", "0", 128);
    }

    if (slrTypeToggleElement.checked == true) {
        setCookie(prefix + "typeSLR", "1", 128);
    }
    else {
        setCookie(prefix + "typeSLR", "0", 128);
    }

    var ratingIndex = getIndexOfCheckedRadioButton(prefix + "viewRating");
    if (ratingIndex != -1) {
        setCookie(prefix + "viewRating", ratingIndex.toString(), 128);
    }

    document.getElementById("viewSettingsPanel").style.display = "none";

    // force reload of page.
    location.reload(true);
}
