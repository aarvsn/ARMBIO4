package jp.co.capcom.android.bio4;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.webkit.WebView;
import android.widget.Button;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

/**
 * OssLicensesActivity — Displays open source software licenses.
 *
 * Reads oss_licenses.json from assets and renders a formatted HTML page
 * listing all third-party open source libraries used by the application.
 * Required for compliance with open source license terms and Google Play policy.
 */
public class OssLicensesActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        WebView webView = new WebView(this);
        setContentView(webView);

        String html = buildLicensesHtml();
        webView.loadDataWithBaseURL(null, html, "text/html", "UTF-8", null);
    }

    private String buildLicensesHtml() {
        StringBuilder sb = new StringBuilder();
        sb.append("<html><head><style>");
        sb.append("body { font-family: sans-serif; padding: 16px; color: #e0e0e0; background: #1a1a1e; }");
        sb.append("h1 { color: #b41e1e; font-size: 20px; }");
        sb.append("h2 { color: #c82319; font-size: 16px; margin-top: 20px; }");
        sb.append("p { font-size: 13px; line-height: 1.5; }");
        sb.append("a { color: #6699cc; }");
        sb.append(".version { color: #888; font-size: 11px; }");
        sb.append("</style></head><body>");
        sb.append("<h1>Open Source Licenses</h1>");
        sb.append("<p>This application uses the following open source software:</p>");

        try {
            InputStream is = getAssets().open("oss_licenses.json");
            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
            StringBuilder jsonStr = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                jsonStr.append(line);
            }
            reader.close();

            JSONArray licenses = new JSONArray(jsonStr.toString());
            for (int i = 0; i < licenses.length(); i++) {
                JSONObject entry = licenses.getJSONObject(i);
                sb.append("<h2>").append(i + 1).append(". ").append(entry.optString("project", "Unknown")).append("</h2>");
                sb.append("<p>").append(entry.optString("description", "")).append("</p>");
                sb.append("<p class='version'>Version: ").append(entry.optString("version", "N/A")).append("</p>");
                sb.append("<p>License: <b>").append(entry.optString("license", "N/A")).append("</b></p>");
                String homepage = entry.optString("homepage", "");
                if (!homepage.isEmpty()) {
                    sb.append("<p><a href='").append(homepage).append("'>Homepage</a></p>");
                }
                String licenseUrl = entry.optString("license_url", "");
                if (!licenseUrl.isEmpty()) {
                    sb.append("<p><a href='").append(licenseUrl).append("'>License Text</a></p>");
                }
            }
        } catch (Exception e) {
            sb.append("<p>Error loading licenses: ").append(e.getMessage()).append("</p>");
        }

        sb.append("<hr>");
        sb.append("<p><b>Disclaimer:</b> Biohazard 4 / Resident Evil 4 is a trademark of ");
        sb.append("Capcom Co., Ltd. This project is NOT affiliated with or endorsed by Capcom. ");
        sb.append("No game assets or copyrighted material from Capcom is included.</p>");
        sb.append("</body></html>");

        return sb.toString();
    }
}
