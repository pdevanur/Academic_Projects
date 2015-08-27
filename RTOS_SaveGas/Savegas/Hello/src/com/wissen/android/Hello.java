package com.wissen.android;

import java.util.ArrayList;
import java.util.List;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.location.Address;
import android.location.Geocoder;
import android.net.Uri;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import com.google.android.maps.GeoPoint;
import com.google.android.maps.MapActivity;
import com.google.android.maps.MapController;
import com.google.android.maps.MapView;
import com.google.android.maps.Overlay;

public class Hello extends MapActivity implements View.OnClickListener {
	/** Called when the activity is first created. */
	LinearLayout 	root 			= null;
	LinearLayout	textLayout 		= null;
	
	EditText		txted1			= null;
	EditText		txted2			= null;
	EditText		txted3			= null;
	EditText		txted4			= null;
	ArrayList<EditText> textBoxes	= null;
	Button			btnAddDest		= null;
	Button			btnShowMap		= null;
	
	List<Address> 	locations		= null;
	MapView			gMapView		= null;
	ArrayList<GeoPoint> points		= null;

    public final int MENU_GO = 1;
    /* Creates the menu items */
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add(0, MENU_GO, 0, "SHOW DIRECTIONS");
        return true;
    }

    /* Handles item selections */
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case MENU_GO:
        	String route = "http://maps.google.com/maps?f=d&saddr=";
    		route += "" + locations.get(0).getLatitude() + "," + locations.get(0).getLongitude() + "&daddr=";
    		route += "" + locations.get(1).getLatitude() + "," + locations.get(1).getLongitude();
    		
    		this.startActivity(new Intent(Intent.ACTION_VIEW,
                    Uri.parse(route))); 
            return true;
        }
        return false;
    }

	protected boolean isRouteDisplayed()
	{
		return true;
	}
	
	class MapOverlay extends com.google.android.maps.Overlay
    {
        @Override
        public boolean draw(Canvas canvas, MapView mapView, boolean shadow, long when) 
        {
            super.draw(canvas, mapView, shadow);                   
            Paint paint = new Paint();
            paint.setStrokeWidth(1);
			paint.setARGB(255, 255, 0, 0);
			paint.setStyle(Paint.Style.STROKE);
            
            //---translate the GeoPoint to screen pixels---
            Point simPoint = new Point();
            for(int i=0; i < points.size(); i++)
            {
            	mapView.getProjection().toPixels(points.get(i), simPoint);
            	
            	//---add the marker---
           
            	Bitmap bmp = BitmapFactory.decodeResource(getResources(), R.drawable.marker);            
            	canvas.drawBitmap(bmp, simPoint.x, simPoint.y, paint);
            	 paint.setStrokeWidth(2);
            	canvas.drawText(Integer.toString(i+1), simPoint.x, simPoint.y, paint);
            }
            return true;
        }
    }
	
	void drawMap()
	{
		setContentView(R.layout.main);
		gMapView = (MapView) findViewById(R.id.myMap);
		//txted1 = (EditText) findViewById(R.id.maptext1);
		points = new ArrayList<GeoPoint>();
		GeoPoint p;
		for(int i=0; i<locations.size(); i++)
		{
			p = new GeoPoint ((int) (locations.get(i).getLatitude() * 1E6), 
									(int) (locations.get(i).getLongitude() * 1E6));
			points.add(p);
			//if(p == null)
			//	txted1.setText("geopoint null");
		}
		
		gMapView.setSatellite(false);
		MapController mc = gMapView.getController();
		//if(mc == null)
		//	txted1.setText("map controller null");
		
		mc.setCenter(points.get(0));
		mc.setZoom(14);
		
		// Adding zoom controls to Map
		LinearLayout zoomLayout =(LinearLayout)findViewById(R.id.layout_zoom);
		View zoomView = gMapView.getZoomControls();
		zoomLayout.addView(zoomView, new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, 
							LayoutParams.WRAP_CONTENT)); 
	    gMapView.displayZoomControls(true);
	    

		MapOverlay mapOverlay = new MapOverlay();
		List<Overlay> listOfOverlays = gMapView.getOverlays();
        listOfOverlays.clear();
        listOfOverlays.add(mapOverlay);  
	}
	
	
	void NNAlgo (List<Address> Addr)
	{
		//Address src = Addr.get(0);
		int iCnt = 0;
		int iNoOfLoc = Addr.size();
		double Lat[] = new double[iNoOfLoc];
		double Lang[] = new double[iNoOfLoc];
		double d, minDist = 0;
		int iIdx;
		
		while (iCnt < iNoOfLoc)
		{
			Lat[iCnt] = Addr.get(iCnt).getLatitude();
			Lang[iCnt] = Addr.get(iCnt).getLongitude(); 
			iCnt++;
		}
		
		for (int i = 0; i < iNoOfLoc; i++)
		{
			iIdx = 0;
			for (int j = i+1; j < iNoOfLoc; j++)
			{
				//use distance formula to identify the distance between the points
				//d=sqrt((x2-x1)^2 + (y2-y1)^2
				d = ((Lat[j]-Lat[i])*(Lat[j]-Lat[i])) + ((Lang[j]-Lang[i])*(Lang[j]-Lang[i])); 
				d = Math.sqrt(d);
				
				if (j == (i+1))
				{
					minDist = d;
					iIdx = j;
				}
				
				else if (minDist > d)
				{
					minDist = d;
					iIdx = j;
				}
			}
			
			//swap the values at [i+1] and [iIdx]
			if ((i+1 != iIdx) && (iIdx != 0)) 
			{
				double temp;
				temp = Lat[i+1];
				Lat[i+1] = Lat[iIdx];
				Lat[iIdx] = temp;
				
				temp = Lang[i+1];
				Lang[i+1] = Lang[iIdx];
				Lang[iIdx] = temp;
				
				Address tempAddr1, tempAddr2;
				tempAddr1 = Addr.get(i+1);
				tempAddr2 = Addr.get(iIdx);
				Addr.remove(i+1);
				Addr.add(i+1, tempAddr2);
				Addr.remove(iIdx);
				Addr.add(iIdx, tempAddr1);
			}
		}
	}
	
	public void onClick(View v) 
	{  
		if(v==btnShowMap) 
		{    
			String addr1 = txted1.getText().toString();  
			String addr2 = txted2.getText().toString();
			
			if(addr1 == "" || addr2 == "")
				return;
			
			Geocoder gc = new Geocoder (this);
			for(int i = 0; i < textBoxes.size(); i++)
			{
				String addr;
				if((addr = textBoxes.get(i).getText().toString()) != "")
					try {
						if(locations == null)
							locations = gc.getFromLocationName(addr, 1);
						else
						{
							List<Address> tmp = gc.getFromLocationName(addr, 1);
							locations.addAll(tmp);
						}
					} catch (Exception e){
						textBoxes.get(i).setText("Illegal Address");
						return;
					}
			}	
			//if(locations.size() != 4)
			//	textBoxes.get(3).setText("not 4 locations");
			NNAlgo(locations);
			//textBoxes.get(3).setText(Integer.toString(locations.size()));
			if(locations.size() >= 2)
				drawMap();
	    }  
		else if(v == btnAddDest)
		{
			for(int i=0; i < textBoxes.size(); i++)
			{
				String s;
				if((s = textBoxes.get(i).getText().toString()) == "")
					return;
			}
			EditText newText = new EditText(this);
			newText.setText("");
			textBoxes.add(newText);
			LinearLayout.LayoutParams tlp = new LinearLayout.LayoutParams( LayoutParams.FILL_PARENT,
												LayoutParams.WRAP_CONTENT );
			root = (LinearLayout) findViewById(R.id.rootLayout);
			textLayout = (LinearLayout) findViewById(R.id.textLayout);
			textLayout.setOrientation( LinearLayout.VERTICAL );
			textLayout.addView( newText, tlp );
 			setContentView( root );
		}
	}  
	
	/* User can zoom in/out using keys provided on keypad */
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_I) {
			gMapView.getController().setZoom(gMapView.getZoomLevel() + 1);
			//gMapView.invalidate();
			return true;
		} else if (keyCode == KeyEvent.KEYCODE_O) {
			gMapView.getController().setZoom(gMapView.getZoomLevel() - 1);
			//gMapView.invalidate();
			return true;
		} else if (keyCode == KeyEvent.KEYCODE_S) {
			gMapView.setSatellite(true);
			return true;
		} else if (keyCode == KeyEvent.KEYCODE_T) {
			gMapView.setTraffic(true);
			return true;
		}
		return false;
	}
	
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.address);
		
		txted1 = (EditText) findViewById(R.id.addr1);
		txted2 = (EditText) findViewById(R.id.addr2);
		//txted3 = (EditText) findViewById(R.id.addr3);
		
		textBoxes = new ArrayList<EditText>();
		txted1.setText("2313 champion court, raleigh, nc 27606");
		txted2.setText("3306 NC-54, Durham, NC 27709");
		//txted3.setText("500 Buck Jones Rd, Raleigh, NC 27606");
		
		textBoxes.add(txted1); 
		textBoxes.add(txted2);
		//textBoxes.add(txted3);
		
		btnAddDest = (Button) findViewById(R.id.addDest);
		btnAddDest.setOnClickListener(this);  		
		
		btnShowMap = (Button) findViewById(R.id.showMap);
		btnShowMap.setOnClickListener(this);
		
	}
}